import sys
import json
import getopt

# Remove 1st argument from the
# list of command line arguments
argumentList = sys.argv[2:]
parameters = sys.argv
#Options 
options = "i:g:c:m:p:s:r:"

long_options = ["Id=", "Gtw=", "Comment=", "ModifiedId=", "PublicKey=", "Status=","RangePower="]

 # Data to be written
routingLine= {
    "id": "",
    "gtw": "",
    "pKey": "",
    "comment": "",
    "status": "",
    "rangePower": ""
}
modifyId = "0"

try:
    arguments, values = getopt.getopt(argumentList, options, long_options)
    
except getopt.error as err:
    print(str(err))

def help():
    print("Arguments:")
    print("     - add need an option -i 'yourId' at least")
    print("     - modify need an option -i 'yourId' at least. You can also add a -m 'yourModifiedId'")
    print("     - delete need an option -i 'yourId' at least")
    print("     - read as no option needed")
    print("\nOptions:")
    print("     - -i 'yourId' is used to found an existing id (or a new one for an add)")
    print("     - -m 'yourModifiedId' is used to modify an existed id")
    print("     - -g 'yourGtw' is used to define your gateway")
    print("     - -p 'yourPublicKey' is used to define your public Key")
    print("     - -s 'yourStatus' is used to define the status of the Node")
    print("     - -r 'yourPowerIndBm' is used to define the Power of the Node in dBm")
    print("\nExit Status:")
    print("     - Return an Error if a problem has occured")

for currentArgument, currentValue in arguments:
        if currentArgument in ("-i", "--Id"):
            routingLine["id"] = currentValue

        elif currentArgument in ("-g", "--Gtw"):
            routingLine["gtw"] = currentValue

        elif currentArgument in ("-c", "--Comment"):
            routingLine["comment"] = currentValue

        elif currentArgument in ("-m", "--ModifiedId"):
            modifyId = currentValue

        elif currentArgument in ("-p", "--PublicKey"):
            routingLine["pKey"] = currentValue

        elif currentArgument in ("-s", "--Status"):
            routingLine["status"] = currentValue
        
        elif currentArgument in ("-r", "--Rangepower"):
            routingLine["rangePower"] = currentValue


if(parameters[1] == "add"):
    print("add")
    if routingLine["id"] != "": # On vérifie qu'il y a bien un Id avant d'ajouter
        if routingLine["status"] == "": # Si aucune valeur n'est assigné alors on prend par défault down
            routingLine["status"] = "down"
        with open('routingTable.json','r') as routingFile:
            jsonFile = json.load(routingFile)
            idExisted = 0
            for item in jsonFile["routingTable"]:
                if item["id"] == routingLine["id"]:
                    idExisted = 1
            # Serializing json
            jsonFile["routingTable"].append(routingLine)
        if idExisted == 0:
            with open('routingTable.json','w') as routingFile:
                # Sets file's current position at offset.
                routingFile.seek(0)
                json.dump(jsonFile, routingFile, indent = 4)
        else:
            print("The id: "+routingLine["id"]+" is already in the routingTable")
    else:
            print("You need to Insert an Id with -i 'your id'")

elif(parameters[1] == "delete"):
    print("delete")
    if routingLine["id"] != "": # On vérifie qu'il y a bien un Id avant d'ajouter
        with open('routingTable.json','r') as routingFile:
            jsonFile = json.load(routingFile)
            modified = 0
            cmp = 0
            for item in jsonFile["routingTable"]:
                if item["id"] == routingLine["id"]:
                    modified = 1
                    del jsonFile["routingTable"][cmp]
                cmp+=1
            if modified == 0:
                print("Error : Id not found in the database")

        with open('routingTable.json','w') as routingFile:
            routingFile.seek(0)
            json.dump(jsonFile, routingFile,  indent = 4)
    else:
        print("You need to choose an Id with -i 'your id'")

elif(parameters[1] == "modify"):
    print("Modify")
    if routingLine["id"] != "": # On vérifie qu'il y a bien un Id avant d'ajouter
        with open("routingTable.json", "r") as routingFile:
            jsonFile = json.load(routingFile)
            modified = 0
            cmp = 0
        with open("routingTable.json", "w") as routingFile:
            for key in jsonFile["routingTable"]:
                if key["id"] == routingLine["id"]:
                    modified = 1
                    if modifyId != "0":
                        jsonFile["routingTable"][cmp]["id"] = modifyId
                    else:
                        jsonFile["routingTable"][cmp]["id"] = routingLine["id"]

                    if routingLine["gtw"] != "":
                        jsonFile["routingTable"][cmp]["gtw"] = routingLine["gtw"]
                        
                    if routingLine["comment"] != "":
                        jsonFile["routingTable"][cmp]["comment"] = routingLine["comment"]

                    if routingLine["pKey"] != "":
                        jsonFile["routingTable"][cmp]["pKey"] = routingLine["pKey"]
                    
                    if routingLine["status"] != "":
                        jsonFile["routingTable"][cmp]["status"] = routingLine["status"]
                    
                    if routingLine["rangePower"] != "":
                        jsonFile["routingTable"][cmp]["rangePower"] = routingLine["rangePower"]
                cmp+=1
            routingFile.seek(0)
            json.dump(jsonFile, routingFile,  indent = 4)
            if modified == 0:
                print("Error : Id not found in the database")
    else:
        print("You need to choose an Id with -i 'your id'")

elif(parameters[1] == "read"):
    with open('routingTable.json','r') as routingFile:
        jsonFile = json.load(routingFile)
        print("read")
        print(jsonFile)

elif(parameters[1] == "-h" or parameters[1] == "--Help"):
    help()