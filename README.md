# Broadmobi IMEI tool
### Small utility to backup and restore your IMEI 

The BM818 wipes its EFS when you update the firmware, setting your IMEI to 0000000.
Use this tool to back it up and be able to restore it after updating it.

*****************************************************************************
    ## WARNING ##
*****************************************************************************
                               
Setting an invalid IMEI can kick you out of a network
Setting an invalid/different IMEI from what came in your device can be illegal in certain countries. Please check [Wikipedia](https://en.wikipedia.org/wiki/International_Mobile_Equipment_Identity#IMEI_and_the_law) for more info
    
#### I'm in no way responsible for any damage done to any device as a result of using this tool.

#### Examples:

##### Backup your data before an update: `imeitool -p /dev/ttyUSB2 -b my_backup`
##### Restore your data after an update: `imeitool -p /dev/ttyUSB2 -r my_backup`


#### Arguments: 

  -p [AT PORT]: The USB AT Port, typically `/dev/ttyUSB2`
  
  -b [FILENAME]: Backup to [FILENAME]
  
  -r [FILENAME]: Restore from [FILENAME]
  
  -h: Show help
  
  -d: Show commands and responses from the modem

### Building it
1. Run `make`

