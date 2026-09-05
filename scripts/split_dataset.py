from collections import Counter
from pathlib import Path
import json
import random

SEED = 20260905
TRAIN_RATIO = 0.70


def read_records(path: Path):
    records = []
    with path.open("r", encoding="utf-8", newline="") as input_file:
        for line_number, line in enumerate(input_file, start=1):
            raw_line = line.rstrip("\r\n")
            if not raw_line.strip():
                continue
            try:
                record = json.loads(raw_line)
            except json.JSONDecodeError as error:
                raise ValueError(f"Invalid JSON on line {line_number}: {error}")
            if not isinstance(record, dict):
                raise ValueError(f"Expected an object on line {line_number}")
            records.append((raw_line, record))
    return records


def write_records(path: Path, records):
    with path.open("w", encoding="utf-8", newline="\n") as output_file:
        for raw_line, _ in records:
            output_file.write(raw_line + "\n")


def label_counts(records):
    return Counter(record["true_label"] for _, record in records)


def main():
    project_root = Path(__file__).resolve().parent.parent
    dataset_directory = project_root / "dataset"
    input_path = dataset_directory / "full_dataset.jsonl"
    train_path = dataset_directory / "train.jsonl"
    holdout_path = dataset_directory / "holdout.jsonl"

    records = read_records(input_path)
    original_count = len(records)

    shuffled_records = list(records)
    random.Random(SEED).shuffle(shuffled_records)
    train_count = int(original_count * TRAIN_RATIO)
    train_records = shuffled_records[:train_count]
    holdout_records = shuffled_records[train_count:]

    if len(train_records) + len(holdout_records) != original_count:
        raise RuntimeError("Split counts do not equal the original dataset size")

    write_records(train_path, train_records)
    write_records(holdout_path, holdout_records)

    print(f"total record count: {original_count}")
    print(f"train count: {len(train_records)}")
    print(f"holdout count: {len(holdout_records)}")
    print(f"train NORMAL/ATTACK: {label_counts(train_records)}")
    print(f"holdout NORMAL/ATTACK: {label_counts(holdout_records)}")
    print(f"random seed: {SEED}")


if __name__ == "__main__":
    main()
