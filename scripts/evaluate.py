import json
import math
import statistics
from pathlib import Path

WINDOW_SECONDS = 60.0
VELOCITY_THRESHOLD = 1.0
COST_PER_FALSE_POSITIVE = 1.0


def clamp_score(value):
    if math.isnan(value):
        return 0.0
    return max(0.0, min(value, 1.0))


def score_record(record):
    request_count = float(record["request_count"])
    velocity_score = clamp_score(
        request_count / WINDOW_SECONDS / VELOCITY_THRESHOLD
    )

    failure_ratio_score = clamp_score(record["failure_ratio"])
    gaps = record["inter_arrival_times"]
    timing_anomaly_score = 0.0

    if len(gaps) >= 2:
        mean = statistics.fmean(gaps)
        if mean > 0.0:
            standard_deviation = statistics.pstdev(gaps)
            coefficient_of_variation = standard_deviation / mean
            timing_suspicion = 1.0 / (1.0 + coefficient_of_variation)
            timing_anomaly_score = clamp_score(
                timing_suspicion * velocity_score
            )

    fingerprint_anomaly_score = clamp_score(
        record["distinct_fingerprint_ratio"]
    )
    concentration_score = 0.0

    final_score = clamp_score(
        0.30 * velocity_score
        + 0.30 * failure_ratio_score
        + 0.20 * timing_anomaly_score
        + 0.15 * fingerprint_anomaly_score
        + 0.05 * concentration_score
    )

    if final_score < 0.30:
        classification = "NORMAL"
    elif final_score < 0.65:
        classification = "SUSPICIOUS"
    else:
        classification = "ATTACK"

    return final_score, classification


def safe_ratio(numerator, denominator):
    return numerator / denominator if denominator else 0.0


def main():
    project_root = Path(__file__).resolve().parent.parent
    holdout_path = project_root / "dataset" / "holdout.jsonl"

    records = []
    with holdout_path.open("r", encoding="utf-8") as input_file:
        for line_number, line in enumerate(input_file, start=1):
            if not line.strip():
                continue
            try:
                records.append(json.loads(line))
            except json.JSONDecodeError as error:
                raise ValueError(
                    f"Invalid JSON on holdout line {line_number}: {error}"
                ) from error

    confusion = {"TP": 0, "TN": 0, "FP": 0, "FN": 0}
    results = []

    for record in records:
        final_score, classification = score_record(record)
        predicted_label = (
            "NORMAL" if classification == "NORMAL" else "ATTACK"
        )
        true_label = record["true_label"]

        if true_label == "ATTACK" and predicted_label == "ATTACK":
            confusion["TP"] += 1
        elif true_label == "NORMAL" and predicted_label == "NORMAL":
            confusion["TN"] += 1
        elif true_label == "NORMAL" and predicted_label == "ATTACK":
            confusion["FP"] += 1
        elif true_label == "ATTACK" and predicted_label == "NORMAL":
            confusion["FN"] += 1
        else:
            raise ValueError(
                f"Unexpected label for {record['source_id']}: "
                f"{true_label}"
            )

        results.append(
            (record["source_id"], true_label, final_score, classification)
        )

    tp = confusion["TP"]
    tn = confusion["TN"]
    fp = confusion["FP"]
    fn = confusion["FN"]
    precision = safe_ratio(tp, tp + fp)
    recall = safe_ratio(tp, tp + fn)
    f1 = safe_ratio(2.0 * precision * recall, precision + recall)
    false_positive_rate = safe_ratio(fp, fp + tn)
    false_negative_rate = safe_ratio(fn, fn + tp)
    false_positive_cost = fp * COST_PER_FALSE_POSITIVE

    print("Held-out evaluation")
    print("===================")
    print(f"Samples: {len(records)}")
    print(f"TP: {tp}")
    print(f"TN: {tn}")
    print(f"FP: {fp}")
    print(f"FN: {fn}")
    print()
    print(f"Precision: {precision:.6f}")
    print(f"Recall: {recall:.6f}")
    print(f"F1: {f1:.6f}")
    print(f"False-positive rate: {false_positive_rate:.6f}")
    print(f"False-negative rate: {false_negative_rate:.6f}")
    print(
        "False-positive cost: "
        f"{false_positive_cost:.6f} synthetic units"
    )
    print()
    print("Per-record summary")
    for source_id, true_label, final_score, classification in results:
        predicted_label = (
            "NORMAL" if classification == "NORMAL" else "ATTACK"
        )
        print(
            f"source_id={source_id} true_label={true_label} "
            f"final_score={final_score:.6f} "
            f"predicted_label={predicted_label} "
            f"classification={classification}"
        )


if __name__ == "__main__":
    main()
