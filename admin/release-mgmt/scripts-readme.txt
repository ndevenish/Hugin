I found these scripts helpful when doing release management.
They are BSD-licensed.

init.sh - initializes the folders layout

    three folders are created. 'trunk' and 'release' are
    relevant for release management. 'branches' is for
    development.

    Edit init.sh to check out the appropriate 'release'
    codeline at the beginning of the release cycle.

build.sh - runs a clean build process

    a folder is wiped out and re-created for the build.
    the relevant files are moved to the top-level folder.

    it still requires manual fiddling, and can be targeted
    to build trunk, release, any of the branches, or
    an extracted tarball.

translate.sh - automatically updates all translations from 'trunk'
               to 'release' and commits.

    one stop solution for translations. make sure you have
    the tool installed as mentioned on the wiki translations
    page.

    pay attention for translations added directly into 'release'
    because this script does not bring them back to 'trunk'.

    watch for the list of files updated during the commit,
    this is the list of translations updated that you want to
    mention in the release notes.

changes.sh - lists changes since last sync

    will list all relevant SVN revision in trunk and saves them to
    release/ChangeLog.update.

    You want to edit ChangeLog.update and filter out / remove
    new features so that only release-relevant fixes are left.

    After running apply.sh, append the remaining ChangeLog.update
    to ChangeLog and commit.

    You also want to mention those changes in the release notes
    as difference to the previous beta / release candidate.

    After running the script, update the LAST variable
    for the next time.

fromtrunk.sh - extract the listed changes from trunk

    edit the list of SVN revisions from changes and run it
    to obtain a series of .diff files.

apply.sh - goes over all diffs extracted from trunk and commit to release

    make sure there are no .diff files other than those
    extracted with fromtrunk.sh in the top level.

    I use .patch as an extension for patches that do not belong
    in the release process.

