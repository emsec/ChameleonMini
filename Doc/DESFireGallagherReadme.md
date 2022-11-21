# Emulating Gallagher on MIFARE DESFire
As of November 2022, Chameleon supports emulation of Gallagher on top of MIFARE DESFire cards. However, there are few limitations. 
Namely, only the AES encryption was tested. Other encryption modes (such as 3K tripple DES) may work if written on the Chameleon using a Proxmark or another reader.
They will, however, NOT work when using the commands below.

Mainly, there is now support for Gallagher self-programming using terminal commands. The supported actions are the following:
* Create a new DESFire card with a Gallagher application
* Select a Gallagher application to perform tasks over
* Create a Gallagher application
* Update a Gallagher application
* Update the card ID of a Gallagher application

However, these things are missing:
* Create a standalone Gallagher application directory app (however, it can be created as a part of creating the whole card)
* Changing the site key (the default site key is used)

For performing all the following commands, you first need to select the current slot to appear as a DESFire card. Eg ``CONFIG=MF_DESFIRE_4KEV1``.

## Create a new DESFire card with a Gallagher application (DF_SETUP_GALL)

## Select a Gallagher application to perform tasks over (DF_SEL_GALLAPP)

## Create a Gallagher application (DF_CRE_GALLAPP)

## Update a Gallagher application (DF_UP_GALLAPP)

## Update the card ID of a Gallagher application (DF_UP_GALL_CID)

## Change the site key (DF_SET_GALLKEY)
This command will return OK WITH TEXT - NOT IMPLEMENTED
