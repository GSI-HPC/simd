# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
#                       Matthias Kretz <m.kretz@gsi.de>

#!/bin/sh
read x <$1
x=$((x+1))
echo $x >$1
echo $x
