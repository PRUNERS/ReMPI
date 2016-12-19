#!/usr/bin/env python
import sys

try_count = 0
broken_count = 0
matched_count = 0

def convert(bin, strace):
    print strace.split("[")

def output(bin):
    for key, val in matched_event.items():
        send_st = convert(bin, key[0])
        recv_st = key[1]
        count   = val[0]
        when    = val[1]
    #    print when, count, send_st, recv_st

def extract_event(rank, line):
    global try_count
    global broken_line
    try_count += 1
    vals = line.split(":")
    try:
        if vals[0].strip() == "STM":
            dic = {}
            dic["id"] = rank
            if not (0 <= dic["id"] and dic["id"] <= 63):
                print "Format error", vals
                exit(1)
            dic["type"] = vals[1]
            if not dic["type"] == "Send" and not dic["type"] == "MF":
                print "Format error", vals
                exit(1)
            dic["rank"] = int(vals[2])
            if not (0 <= dic["rank"] and dic["rank"] <= 63):
                print "Format error", vals
                exit(1)
            dic["tag"] = int(vals[3])
            dic["st"] = vals[4]
            return dic
        else:
            return None
    except:
        print vals
        broken_count += 1
        return None
    
matched_event = {}
def add_matched_event(send_event, recv_event):
    global matched_count
    matched_count += 1
    tuple  = (send_event["st"], recv_event["st"])
    if matched_event.has_key(tuple) == False:
        matched_event[tuple] = [1, matched_count]
    else:
        matched_event[tuple][0] = matched_event[tuple][0] + 1
    

def message_matching(e1, e2):
    if e1["type"] == "Send" and e2["type"] == "MF":
        send_event = e1
        recv_event = e2
    elif e1["type"] == "MF" and e2["type"] == "Send":
        send_event = e2
        recv_event = e1
    else:
        return False

    if send_event["rank"] == recv_event["id"] and send_event["id"] == recv_event["rank"] and send_event["tag"] == recv_event["tag"]:
        add_matched_event(send_event, recv_event)
        return True
    else:
        return False
 
read_size = 0
def read_event(rank):
    global read_size
    line = fds[rank].readline()
    read_size += len(line)
    if line == "":
        return None
    else:
        return extract_event(rank, line)

fds = []
def open_files(num):
    global fds
    for i in range(num):
        fd = open("./rank_" + str(i) + ".stmx")
        fds.append(fd)


def pop_next_event(rank):
    next_event = None
    event_cache = event_caches[rank]
    if len(event_cache) > 0:
        next_event = event_cache.pop(0)
    else:
        next_event = read_event(rank)
    return next_event

def event_matching(event, target_rank):
    target_event_cache = event_caches[target_rank]
    for target_event in target_event_cache:
        is_matched = message_matching(event, target_event)
        if is_matched: 
            target_event_cache.remove(target_event)
            return True
    target_event = read_event(target_rank)
    while not target_event == None:
        is_matched = message_matching(event, target_event)

        if is_matched == True: return True
        event_caches[target_rank].append(target_event)
        target_event = read_event(target_rank)
    return False

orphans=[]

event_caches = None
def match_message_of(rank):
    event = pop_next_event(rank)

    if event == None:
        return True
    is_matched = event_matching(event, event["rank"])
    if is_matched == False:
        orphans.append(event)
    
def sum_cache():
    sum = 0
    for event_cache in event_caches:
        sum += len(event_cache)
    return sum

def main():
    global event_caches
    global read_size
    global matched_count
    num_rank = 64
    event_caches = [[] for j in range(num_rank)]
    
    if len(sys.argv) != 2:
        print "error"
        exit(1)
    bin = sys.argv[1]

    open_files(num_rank)


    while True:
        done_count = 0
        for rank in range(num_rank):
            done = match_message_of(rank)
            if done == True: done_count += 1
        print float(read_size) / 1000000000, "GB", 
        print matched_count, done_count, broken_count, len(orphans), sum_cache()
        if done_count == num_rank:
            break

    output(bin)



if __name__ == "__main__":
    main()
