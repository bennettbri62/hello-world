import os
import sys
import requests
import json

b2BucketName = os.getenv('NAME')
# Note that we won't include a leading /...
b2FileName = "mnt/c/Users/Brian Bennett-admin/Desktop/base.py"
# Note that in this case we will include the leading / for an absolute path...
localFilename = "/mnt/c/Download/base.py"

keyID = os.getenv("B2_APPLICATION_KEY_ID")
applicationKey = os.getenv("B2_APPLICATION_KEY")

response = requests.get("https://api.backblazeb2.com/b2api/v2/b2_authorize_account", auth=(keyID, applicationKey))

if response.status_code!=200:
    print("Bad response from b2_authorize_account: " + str(response.status_code))
    sys.exit(4)
    

responseDataDict = json.loads(response.content)
downloadUrl = responseDataDict["downloadUrl"]
authorizationToken = responseDataDict["authorizationToken"]

# Now download a file...
headers = {"Authorization": authorizationToken}
response = requests.get(downloadUrl + '/file/DELL102/mnt/c/Users/Brian Bennett-admin/Desktop/base.py', headers=headers)
if response.status_code!=200:
    print("Bad response from b2 download: " + str(response.status_code))
    sys.exit(4)

# response.text contains our file content...
# print(response.text)
print(type(response.text))

text_file = open(localFilename, "w")
text_file.write(response.text)
text_file.close()
