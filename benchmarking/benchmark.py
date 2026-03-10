import argparse
import csv
from filesystem import Ext4FileSystem, Ext4FuseFileSystem, FloudsFileSystem
from workload import Workload

# Main function that parses arguments, runs workloads, and saves results to csv
def main():
    default_workloads = ["append_small_1000", "create_dirs_deep_20000", "create_dirs_flat_20000", "create_small_5000", "delete_small_5000", "dirops_deep_5000", "dirops_flat_5000", "fileserver_read_500", "fileserver_read_5000", "open_close_5000", "remove_dirs_deep_20000", "remove_dirs_flat_20000", "rread_1g", "rwrite_1g", "seqread_1g", "seqwrite_1g",]

    parser = argparse.ArgumentParser(description="Benchmarking Suite")
    parser.add_argument("--target", required=True, choices=["ext4", "ext4_fuse", "flouds"], help="Target filesystem to benchmark")
    parser.add_argument("--workloads", nargs='*', help="Specific workloads to run (default: all)", choices=default_workloads)
    parser.add_argument("--output", help="Output file for results", default="benchmark_results.csv")

    args = parser.parse_args()

    filesystem = None
    if args.target == "ext4":
        filesystem = Ext4FileSystem()
    elif args.target == "ext4_fuse":
        filesystem = Ext4FuseFileSystem()
    elif args.target == "flouds":
        filesystem = FloudsFileSystem()
    
    # If no specific workloads provided, run all default workloads
    workloads = args.workloads if args.workloads else default_workloads

    # Run each workload and collect results
    results = []
    for workload_name in workloads:
        # Run each workload 5 times and average results
        workload_ops = []
        workload_used_space = []

        for _ in range(5):
            workload = Workload(workload_name, filesystem)
            workload.run()
            workload_ops.append(workload.ops_per_sec)
            workload_used_space.append(workload.used_space)

        results.append({
            'workload': workload_name,
            'ops_per_sec': sum(workload_ops) / len(workload_ops),
            'ops_per_sec_standard_deviation': (sum((x - (sum(workload_ops) / len(workload_ops))) ** 2 for x in workload_ops) / len(workload_ops)) ** 0.5,
            'used_space': sum(workload_used_space) / len(workload_used_space),
            'used_space_standard_deviation': (sum((x - (sum(workload_used_space) / len(workload_used_space))) ** 2 for x in workload_used_space) / len(workload_used_space)) ** 0.5,
        })

    # Write results to CSV
    with open(args.output, 'w', newline='') as csvfile:
        fieldnames = ['workload', 'ops_per_sec', 'ops_per_sec_standard_deviation', 'used_space', 'used_space_standard_deviation']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        for row in results:
            writer.writerow(row)

if __name__ == "__main__":
    exit(main())
