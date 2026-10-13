import json
import os
import py_compile
import subprocess
import sys
import tempfile
from huggingface_hub import hf_hub_download
from llama_cpp import Llama
from pydantic import BaseModel, Field
import requests

# 1. Modell herunterladen oder aus Cache laden
os.makedirs("./models", exist_ok=True)
model_path = "./models/qwen2.5-coder-7b-instruct-q4_k_m.gguf"

if not os.path.exists(model_path):
    print("Lade 7B Q4_K_M GGUF herunter...")
    hf_hub_download(
        repo_id="Qwen/Qwen2.5-Coder-7B-Instruct-GGUF",
        filename="qwen2.5-coder-7b-instruct-q4_k_m.gguf",
        local_dir="./models",
    )
else:
    print("7B Modell direkt aus Cache bereitgestellt.")

# 2. Pydantic-Schema für strikte Grammatik
class CodeTriageDecision(BaseModel):
    has_bug: bool = Field(
        description="True ONLY if real security issue or broken edge case exists"
    )
    category: str = Field(description="Category name")
    what_if_scenario: str = Field(
        description="1-2 sentences on what goes wrong"
    )
    fixed_code: str = Field(description="Exact replacement lines of code")

token = os.environ.get("GITHUB_TOKEN")
repo = os.environ.get("REPO_FULL_NAME")
pr_num = os.environ.get("PR_NUMBER")
head_sha = os.environ.get("COMMIT_SHA")

# 3. Diff und geänderte Python-Dateien ermitteln
try:
    diff_out = subprocess.check_output(
        ["git", "diff", "-U0", "origin/master...HEAD"], text=True
    )
except Exception:
    try:
        diff_out = subprocess.check_output(
            ["git", "diff", "-U0", "HEAD~1...HEAD"], text=True
        )
    except Exception:
        diff_out = ""

if not diff_out.strip():
    print("VibeCI: Kein Diff vorhanden. Triage beendet.")
    sys.exit(0)

target_tasks = []
curr_file = None
for line in diff_out.splitlines():
    if line.startswith("+++ b/"):
        curr_file = line[6:]
    elif line.startswith("@@ ") and curr_file and curr_file.endswith(".py"):
        parts = line.split(" ")
        if len(parts) > 2 and parts[2].startswith("+"):
            info = parts[2][1:].split(",")
            target_tasks.append((curr_file, int(info[0])))

if not target_tasks:
    print("VibeCI: Keine modifizierten Python-Dateien gefunden.")
    sys.exit(0)

# 4. Scanner-Ergebnisse laden
diag = {"linter": []}
if os.path.exists("ruff-report.json"):
    try:
        with open("ruff-report.json") as f:
            diag["linter"] = json.load(f)[:5]
    except Exception:
        pass

# 5. Lokales Modell initialisieren
print("Lade 7B Modell in den Runner-RAM...")
llm = Llama(
    model_path=model_path,
    n_ctx=2048,
    n_threads=2,
    n_batch=512,
    verbose=False,
)

review_comments = []

for rel_path, line_no in target_tasks[:3]:
    if not os.path.exists(rel_path):
        continue

    with open(rel_path, "r") as f:
        lines = f.readlines()

    start = max(0, line_no - 12)
    end = min(len(lines), line_no + 12)
    snippet = "".join(lines[start:end])

    messages = [
        {
            "role": "system",
            "content": "You are a Security Auditor. Perform What-If triage on edge cases. If safe, set has_bug: false.",
        },
        {
            "role": "user",
            "content": f"Diagnostics:\n{json.dumps(diag)}\n\nFile: {rel_path} (Line {line_no}):\n```python\n{snippet}\n```",
        },
    ]

    try:
        res = llm.create_chat_completion(
            messages=messages,
            response_format={
                "type": "json_object",
                "schema": CodeTriageDecision.model_json_schema(),
            },
            temperature=0.1,
            max_tokens=350,
        )

        payload = json.loads(res["choices"][0]["message"]["content"])

        if payload.get("has_bug") and payload.get("fixed_code"):
            fix = payload["fixed_code"].strip()
            if fix.startswith("```python"):
                fix = fix[9:].strip()
            if fix.endswith("```"):
                fix = fix[:-3].strip()

            # Anti-Halluzinations Gate: Syntax prüfen
            with tempfile.NamedTemporaryFile(
                suffix=".py", mode="w", delete=False
            ) as tmp:
                tmp.write(fix)
                tmp_name = tmp.name

            try:
                py_compile.compile(tmp_name, doraise=True)
                os.remove(tmp_name)

                ticks = chr(96) * 3
                sug_block = f"{ticks}suggestion\n{fix}\n{ticks}"
                body = (
                    f"### 🛡️ VibeCI Security Audit: `{payload.get('category')}`\n\n"
                    f"**Was-wäre-wenn-Szenario:** {payload.get('what_if_scenario')}\n\n"
                    f"{sug_block}"
                )

                review_comments.append({
                    "path": rel_path,
                    "line": line_no,
                    "side": "RIGHT",
                    "body": body,
                })
                print(f"Vorschlag für {rel_path}:{line_no} vorbereitet.")
            except py_compile.PyCompileError as e:
                print(f"Anti-Halluzination Gate hat ungültigen Code blockiert: {e}")
                if os.path.exists(tmp_name):
                    os.remove(tmp_name)

    except Exception as err:
        print(f"Inferenz-Fehler bei {rel_path}:{line_no}: {err}")

# 6. Kommentare an GitHub API senden
if review_comments:
    print(f"Sende {len(review_comments)} Vorschläge an PR #{pr_num}...")
    api_url = f"https://github.com{repo}/pulls/{pr_num}/reviews"
    headers = {
        "Authorization": f"Bearer {token}",
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
    }
    data = {
        "commit_id": head_sha,
        "body": "🤖 **VibeCI Autonomous Audit:** Schwachstellen identifiziert. Übernimm die Vorschläge per Klick auf Commit suggestion.",
        "event": "COMMENT",
        "comments": review_comments,
    }
    r = requests.post(api_url, headers=headers, json=data)
    print("GitHub API Status:", r.status_code)
else:
    print("Keine Lücken gefunden. Kein PR-Kommentar erforderlich.")
