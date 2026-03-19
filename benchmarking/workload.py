import os
import subprocess

# Class representing a filebench workload that setups the filesystem, runs the workload and extracts results
class Workload:
    def __init__(self, name, filesystem):
        self.name = name
        self.filesystem = filesystem
        self.ops_per_sec = None
        self.used_space = None

    def run(self):
        # Setup the filesystem
        self.filesystem.setup()
        print(f"Running workload {self.name} on {self.filesystem.__class__.__name__}")

        # Run filebench
        workload_file = os.path.join(os.path.dirname(__file__), "workloads", f"{self.name}.f")
        result = subprocess.run(
            ["filebench", "-f", workload_file],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )
        output = result.stdout

        # Extract IO Summary line and ops/s value
        self.ops_per_sec = None
        for line in output.splitlines():
            if "IO Summary" in line:
                io_summary = line.strip()
                parts = io_summary.split(',')
                for part in parts:
                    if "ops/s" in part:
                        try:
                            self.ops_per_sec = float(part.strip().split()[0])
                        except Exception:
                            self.ops_per_sec = None
                        break
                break

        if self.ops_per_sec is not None:
            print(f"Extracted ops/s: {self.ops_per_sec}")
        else:
            print("Could not extract ops/s value. Filebench output:")
            print(output)

        self.used_space =  self.filesystem.used_space()
        print(f"Used space after workload: {self.used_space} bytes")

        self.filesystem.teardown()