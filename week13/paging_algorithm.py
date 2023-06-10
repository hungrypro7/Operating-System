print("FIFO")
def FIFO(size, pages):
    SIZE = size
    memory = []
    faults = 0
    for page in pages:
        if memory.count(page) == 0 and len(memory) < SIZE:  # 만약 해당 원소가 없거나, memory 길이가 SIZE 보다 작다면
            memory.append(page)     # 다음 원소 추가
            faults += 1             # fault 증가
        elif memory.count(page) == 0 and len(memory) == SIZE:   # 만약 해당 원소가 없거나, memory 길이가 SIZE 이면
            memory.pop(0)       # 맨 첫번째 원소를 OUT
            memory.append(page)     # 다음 원소 추가
            faults += 1             # fault 증가
        print(memory)
    print(faults, "page faults")
    return faults

a = [7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1]
FIFO(3, a)

print("======================================")
print("LRU")

def LRU(size, pages):
    SIZE = size
    memory = []
    faults = 0
    for page in pages:
        if memory.count(page) == 0 and len(memory) < SIZE:
            memory.append(page)
            faults += 1
        elif memory.count(page) == 0 and len(memory) == SIZE:
            memory.pop(0)
            memory.append(page)
            faults += 1
        elif memory.count(page) > 0:
            memory.remove(page)
            memory.append(page)
        print(memory)
    print(faults, "page faults")
    return faults

LRU(3, a)

print("======================================")

print("OPT")

def OPT(size, pages):
    SIZE = size
    memory = []
    faults = 0
    for i in range(len(pages)):
        if memory.count(pages[i]) == 0 and len(memory) < SIZE:
            memory.append(pages[i])
            faults += 1
        elif memory.count(pages[i]) == 0 and len(memory) == SIZE:       # page에 없을 때
            count = [0] * 3
            insert_position = 0
            for j in range(i+1, len(pages)):
                if count.count(0) == 1:
                    insert_position = count.index(0)
                    break
                if pages[j] in memory:
                    count[memory.index(pages[j])] = 1
            memory[insert_position] = pages[i]
            faults += 1
        print(memory)
    print(faults, "page faults")
    return faults

a = [7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1]
OPT(3, a)