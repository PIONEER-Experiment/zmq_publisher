"""Online pipeline defaults for the repository's synthetic MIDAS frontends."""

import os
from pathlib import Path
import sys

# Real MuSiP runs obtain geometry from the run configuration. The self-contained
# simulator instead installs a small synthetic geometry before configuring the
# ordinary online pipeline.
os.environ["PIONEER_GAUDI_MUSIP_DEMO_GEOMETRY"] = "1"

sys.path.insert(0, str(Path(__file__).resolve().parent))
from online_pipeline import *  # noqa: F401,F403,E402
