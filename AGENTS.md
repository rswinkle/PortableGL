

Unless I specifically say otherwise, you can ignore the scratch directory, it's full of quick tests, prototypes, notes, and not
included as part of the git repo for a reason.


RULES
=====
*Do not proactively edit files before I have given the greenlight to do something. If I have given the go-ahead
you can edit what needs to be edited to accomplish the task I've given.

*Do not proactively remove comments unless you *know* they are redundant/useless due to changes you are making.

*Try to make all your scripting paths relative so I don't get hit with a request to access outside of the repo directory
because I will almost always deny it and then you'll have to reformulate the commands anyway.

*Do not use always-on checks for invariants that should never be broken in normal
usage; those clutter the code and slow down release builds. Use asserts instead.
I would rather have an immediate crash during development that reveals a problem
than a safety check that papers over broken state or soft-fails and continues.

*Always check whether your changes work with other build configurations, especially the release/optimized build.
You don't necessarily have to build every one, but at least logically check that you're not relying on any code
that is eliminated in the release build.


