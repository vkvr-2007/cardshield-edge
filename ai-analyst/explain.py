import argparse
import json
import os
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


REQUIRED_FIELDS = {
    "source_id",
    "request_count",
    "failure_count",
    "failure_ratio",
    "velocity_score",
    "timing_anomaly_score",
    "fingerprint_anomaly_score",
    "concentration_score",
    "final_score",
    "classification",
}
API_URL = "https://api.openai.com/v1/chat/completions"
DEFAULT_MODEL = "gpt-4o-mini"
ALLOWED_ACTIONS = (
    "monitor",
    "recommend rate-limit",
    "escalate to merchant",
)


def load_evidence(value):
    path = Path(value)
    if path.is_file():
        text = path.read_text(encoding="utf-8")
    else:
        text = value

    try:
        evidence = json.loads(text)
    except json.JSONDecodeError as error:
        raise ValueError(f"Evidence is not valid JSON: {error}") from error

    if not isinstance(evidence, dict):
        raise ValueError("Evidence must be one JSON object")

    missing = sorted(REQUIRED_FIELDS - evidence.keys())
    if missing:
        raise ValueError(
            "Evidence is missing required fields: " + ", ".join(missing)
        )
    return evidence


def build_prompt(evidence):
    evidence_json = json.dumps(evidence, sort_keys=True)
    actions = ", ".join(f'"{action}"' for action in ALLOWED_ACTIONS)
    return f"""You are the CardShield Edge analyst layer for defensive security analysis only.
The C++ detector already made the risk decision represented by the classification
and score in the evidence below. Do not make or change the detection decision.
Do not invent evidence, do not claim certainty, and do not recommend blocking,
banning, deleting, or any other aggressive automated action.

Using only the supplied structured evidence, provide exactly:
1. A 2-3 sentence plain-English explanation of why this source was flagged.
2. Explicit reference to the strongest relevant behavioral signals.
3. One recommended_action chosen exactly from: {actions}.

Return concise plain text with the action on its own line as:
recommended_action: <one allowed action>

Evidence:
{evidence_json}
"""


def call_llm(evidence, api_key, model):
    payload = {
        "model": model,
        "messages": [
            {
                "role": "system",
                "content": (
                    "You explain detector evidence for a defensive security "
                    "analyst. The detector, not you, makes risk decisions."
                ),
            },
            {"role": "user", "content": build_prompt(evidence)},
        ],
        "temperature": 0.2,
    }
    request = Request(
        API_URL,
        data=json.dumps(payload).encode("utf-8"),
        headers={
            "Authorization": f"Bearer {api_key}",
            "Content-Type": "application/json",
        },
        method="POST",
    )

    try:
        with urlopen(request, timeout=60) as response:
            response_data = json.load(response)
    except HTTPError as error:
        detail = error.read().decode("utf-8", errors="replace")
        raise RuntimeError(f"LLM API request failed ({error.code}): {detail}")
    except URLError as error:
        raise RuntimeError(f"Could not reach LLM API: {error.reason}") from error

    try:
        return response_data["choices"][0]["message"]["content"]
    except (KeyError, IndexError, TypeError) as error:
        raise RuntimeError("LLM API response did not contain message content") from error


def main():
    parser = argparse.ArgumentParser(
        description="Explain existing CardShield Edge risk evidence with an LLM."
    )
    parser.add_argument(
        "evidence",
        help="Path to an evidence JSON file or an inline JSON object",
    )
    parser.add_argument(
        "--model",
        default=os.environ.get("OPENAI_MODEL", DEFAULT_MODEL),
        help=f"OpenAI model (default: {DEFAULT_MODEL})",
    )
    args = parser.parse_args()

    api_key = os.environ.get("OPENAI_API_KEY")
    if not api_key:
        print(
            "OPENAI_API_KEY is not set. Set it before running this demo, "
            "for example: $env:OPENAI_API_KEY = 'your-key'",
            flush=True,
        )
        return 2

    try:
        evidence = load_evidence(args.evidence)
        explanation = call_llm(evidence, api_key, args.model)
    except (OSError, ValueError, RuntimeError) as error:
        print(f"Error: {error}")
        return 1

    print(explanation)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
