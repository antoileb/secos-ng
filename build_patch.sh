#!/bin/sh

ARCHIVE_NAME=$1

# Add remote Anaïs Gantet secos-ng repository
git remote add upstream git@github.com:agantet/secos-ng.git

# Fetch last updates of the repository
git fetch upstream

# Build patch
git format-patch upstream/main..exam --stdout > tp_exam.patch

# Build the archive
tar czf $ARCHIVE_NAME.tar.gz *.patch