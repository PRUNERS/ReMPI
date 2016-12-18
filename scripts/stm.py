#!/bin/env python
import sys

def extract_event(line):
    vals = line.split(":")
    if vals[4].strip() == "STM":
        dic = {}
        dic["id"] = vals[3]
        dic["type"] = vals[5]
        dic["rank"] = vals[6]
        dic["tag"] = vals[7]
        dic["st"] = vals[8]
        return dic
    else:
        return None

event_cache = []

def message_matching(e1, e2):
    if e1["type"] == "Send" && e2["type"] == "MF":
        send_event = e1
        recv_event = e2
    elif e1["type"] == "MF" && e2["type"] == "Send":
        send_event = e2
        recv_event = e1
    else:
        return False

def match_message_in_cache(event):
    for e in event_cache:
        if event["type"] == e["type"]:
            continue
        message_matching(event, e)


def match_message(fd, event):
    match_message_in_cache(event)
    

def main():
    # my code here
    if len(sys.argv) == 3:
        input_path = sys.argv[1]
        output_path = sys.argv[2]
    elif len(sys.argv) == 2:
        input_path = sys.argv[1]
        output_path = None

    fd = open(input_path, "r")
    for line in fd:
        event = extract_event(line);
        if event == None: 
            continue
        match_message(fd, event)


if __name__ == "__main__":
    main()
