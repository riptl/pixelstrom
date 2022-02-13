from dataclasses import dataclass
import json
from pathlib import Path
import subprocess

import yaml


@dataclass
class CanvasCase:
    id: str
    input: dict
    result: dict

    def write_input(self, input_path: Path):
        with open(input_path, "w") as input_file:
            json.dump(self.input, input_file)


def read_canvas_cases(case_path: Path) -> dict:
    with open(case_path, "r") as case_file:
        docs = yaml.safe_load_all(case_file)
        return [CanvasCase(**obj) for obj in docs]


def get_canvas_cases():
    tests_dir = Path(__file__).parent
    cases = []
    for case_path in tests_dir.iterdir():
        if not case_path.name.endswith(".yml"):
            continue
        cases += read_canvas_cases(case_path)
    return cases, [c.id for c in cases]


def pytest_generate_tests(metafunc):
    if "canvas_case" in metafunc.fixturenames:
        cases, ids = get_canvas_cases()
        metafunc.parametrize("canvas_case", cases, ids=ids)


def test_canvas(tmp_path: Path, canvas_case: CanvasCase):
    input_path = tmp_path / "input.json"
    canvas_case.write_input(input_path)

    bin_path = Path(__file__).parent.parent / "dist" / "canvas.so"
    cmd = subprocess.run(
        [
            "rbpf-cli",
            "--output=json-compact",
            str(bin_path),
            "-i",
            input_path,
        ],
        text=True,
        stdout=subprocess.PIPE,
    )

    assert cmd.returncode == 0

    output = json.loads(cmd.stdout)
    del output["execution_time"]
    assert output == canvas_case.result
