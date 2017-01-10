# boss

This is an example for using SpotPass. This can only be used if bosshaxx from ctr-httpwn >=v1.2 is active, or if you're using "CFW"(when using unsigned SpotPass content).

When running under \*hax payload, the required \*hax payload version is >=v2.8. In that case, the registered BOSS task is associated with Home Menu, with the SpotPass content being stored under the NAND extdata specified by Home Menu for SpotPass data storage. The task+content are automatically deleted by this example once finished. Due to being associated with Home Menu, seperate bossdata URLs for each region is required.

If you prefer to use BOSS with the title your application is running under instead, you can use this: <code>bossInit(0, true);</code> You might have to use bossSetStorageInfo() if BOSS wasn't setup for use with that title.

See build_bossdata.sh for building the bossdata files. NOTE: bosstool currently uses a hard-coded timestamp for building the BOSS-container, it's unknown whether this needs to be changed for tasks that run more than once when updating the content.
