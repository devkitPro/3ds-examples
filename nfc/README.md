This example is for using NFC for accessing amiibo.

The amiibo AppID for AppData used here is hard-coded for Super Smash Bros. This example saves the read amiibo AppData to SD, and can write AppData from SD to the amiibo as well.

The \*hax payload will automatically handle running this under the required process for using the "nfc:u" service, on New3DS. What title to run under on Old3DS hasn't been determined yet(Old3DS doesn't have any system-applications that could be used for this unlike New3DS). With Old3DS the target would need nfcu service access, and bitmask 0xF0000000 clear in the tidlow for the NFC-sysmodule dependency in the exheader.

