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
This command sets up a new Gallagher encoded DESFire card with a single application. This is usually enough to emulate a personal access card.
If you just need a card copy, this is the only Gallagher command you need. If you need to change the UID of the Chameleon, remember to do it before running this command.

The syntax is: ``DF_SETUP_GALL=C<cardNumber>F<facilityId>I<issuieLevel>R<regionCode>`` eg ``DF_SETUP_GALL=C123456F1234I1R2``.

Card ID is a 32bit unsigned integer, facility ID is a 16bit unsigned integer and issue level as well as region code are 8bit unsigned integers.

## Select a Gallagher application to perform tasks over (DF_SEL_GALLAPP)
Selects the AID of the Gallagher app to perform operations over. This is needed for ``DF_CRE_GALLAPP``, ``DF_UP_GALLAPP``, and ``DF_UP_GALL_CID``. You do not need to run this command if you ran ``DF_SETUP_GALL`` previously as the default value is the last app an operation was executed over.

The syntax is: ``DF_SEL_GALLAPP=<AID>`` eg ``DF_SEL_GALLAPP=<A1B2C3>`` or ``DF_SEL_GALLAPP=<000100>``.

The AID has three bytes and all six hexadecimal digits need to be present when running this command.

## Create a Gallagher application (DF_CRE_GALLAPP)
Run ``DF_SEL_GALLAPP`` or ``DF_SETUP_GALL`` first!

This command sets up the Gallagher application only. It does not set up or update the application directory app. The syntax is the same as with ``DF_SETUP_GAL``. If you want to update data in an already existiing application, use ``DF_UP_GALLAPP``.

Warning! You need to set up the Gallagher application directory app yoursef. However, the DESFire card application directory will be updated for you.

## Update a Gallagher application (DF_UP_GALLAPP)
Run ``DF_SEL_GALLAPP`` or ``DF_SETUP_GALL`` first!

This command lets you update the contents of a Gallagher application. The syntax is the same as with ``DF_SETUP_GAL``. If you want to create a new application, use ``DF_CRE_GALLAPP``.

## Update the card ID of a Gallagher application (DF_UP_GALL_CID)
Run ``DF_SEL_GALLAPP`` or ``DF_SETUP_GALL`` first!

This command lets you update the Gallagher card ID of a already existing Gallagher app. The facility ID, region and issue level are taken from the last operation. The AID is either taken from the last app create operation or from ``DF_SEL_GALLAPP``.

The syntax is ``DF_UP_GALL_CIDP=<cardId>``. Eg. ``DF_UP_GALL_CIDP=123456``. The card ID is a 32bit unsigned integer.

## Change the site key (DF_SET_GALLKEY)
This command will return OK WITH TEXT - NOT IMPLEMENTED
