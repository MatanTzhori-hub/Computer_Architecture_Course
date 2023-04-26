import random

## Define defaults
btbSize = 2
historySize = 2
tagSize = 26
fsmState = 1
isGlobalHist = "local_history"    # local_history / global_history
isGlobalTable = "local_tables"   # local_tables / global_tables
Shared = "using_share_lsb"         # not_using_share / using_share_lsb / using_share_mid
## -----------------

branches = ["0x1230", "0x87654", "0x4074", "0x3018"]
destinations = ["0x12300", "0x45678", "0x17044", "0x13408"]
taken = ["T" ,"N"]

first_line = [btbSize,historySize,tagSize,fsmState,isGlobalHist,isGlobalTable,Shared]
first_line = " ".join(map(str, first_line))

file = open("myexample1.trc", "w")
file.writelines(first_line)

for branch in range(1, 501, 1):
    file.writelines("\n")
    rnd_branch = random.randint(0, len(branches)-1)
    rnd_choise = random.randint(0, len(taken)-1)
    new_line = " ".join(map(str, [branches[rnd_branch], taken[rnd_choise], destinations[rnd_branch]]))
    file.writelines(new_line)

file.close()


