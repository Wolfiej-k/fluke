#!/usr/bin/env python3
import sys
import re

def main():
    if len(sys.argv) != 2:
        print("") 
        return

    log_file = sys.argv[1]
    safe_ids = []
    
    # Regex to find "id=x Result: OK"
    # Adjusts to handle varying whitespace
    regex = re.compile(r"id=(\d+)\s+Result:\s+OK")

    try:
        with open(log_file, 'r') as f:
            for line in f:
                match = regex.search(line)
                if match:
                    safe_ids.append(match.group(1))
    except FileNotFoundError:
        print("")
        return

    # Output "1,7,8" to stdout
    print(",".join(safe_ids))

if __name__ == "__main__":
    main()
