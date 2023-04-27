import random

## Define defaults
btbSize = 4
historySize = 8
tagSize = 14
fsmState = 1
isGlobalHist = "local_history"    # local_history / global_history
isGlobalTable = "global_tables"   # local_tables / global_tables
Shared = "using_share_lsb"         # not_using_share / using_share_lsb / using_share_mid
## -----------------

branches = ["0x1230", "0x87654", "0x4074", "0x3018", "0x157c", "0x24b0"]#
destinations = ["0x12300", "0x45678", "0x17044", "0x13408", "0x16a40", "0x20f8"]#
taken = ["T" ,"N"]

first_line = [btbSize,historySize,tagSize,fsmState,isGlobalHist,isGlobalTable,Shared]
first_line = " ".join(map(str, first_line))

file = open("myexample1.trc", "w")
file.writelines(first_line)

for branch in range(1, 10001, 1):
    file.writelines("\n")
    rnd_branch = random.randint(0, len(branches)-1)
    rnd_choise = 0  #random.randint(0, len(taken)-1)
    new_line = " ".join(map(str, [branches[rnd_branch], taken[rnd_choise], destinations[rnd_branch]]))
    file.writelines(new_line)

file.close()


