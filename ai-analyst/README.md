# CardShield Edge AI Analyst Demo

This is a minimal analyst-layer demo. Detection happens in the existing C++
CardShield Edge engine. The LLM only explains structured evidence that the
C++ detector has already produced; it does not make the risk decision.

The analyst may recommend only one of these actions:

- `monitor`
- `recommend rate-limit`
- `escalate to merchant`

It must not recommend blocking, banning, deleting, or another aggressive
automated action.

## Provider Setup

This demo uses the OpenAI Chat Completions API. Set the API key in the
process environment. The key is never stored in the repository:

```powershell
$env:OPENAI_API_KEY = "your-api-key"
```

The default model is `gpt-4o-mini`. Override it with `OPENAI_MODEL` or the
`--model` option.

## Run

From the repository root, pass either an evidence JSON file or an inline JSON
object:

```powershell
python scripts/evaluate.py
python ai-analyst/explain.py path\to\evidence.json
python ai-analyst/explain.py '{"source_id":"src_001","request_count":61,"failure_count":49,"failure_ratio":0.80,"velocity_score":1.0,"timing_anomaly_score":0.9,"fingerprint_anomaly_score":1.0,"concentration_score":0.0,"final_score":0.87,"classification":"ATTACK"}'
```

The evidence object must contain the evaluator fields required by
`explain.py`. The script sends only that evidence to the LLM and prints the
model's explanation and allowed recommended action.
