"""
UE5 Python startup script for PoF project.
This file is automatically executed by UE5's Python plugin when the editor starts
(if configured in Project Settings > Python > Startup Scripts).

It adds the Content/Python directory to the Python path so our Mixamo pipeline
modules can be imported from anywhere (console, commandlet, editor widget).
"""

import sys
import os
import unreal

# Ensure our Content/Python directory is on the path
project_python_dir = os.path.join(unreal.Paths.project_content_dir(), "Python")
if project_python_dir not in sys.path:
    sys.path.insert(0, project_python_dir)
    unreal.log(f"Added to Python path: {project_python_dir}")

unreal.log("PoF Python modules available: mixamo_import, mixamo_retarget, mixamo_batch_retarget, mixamo_root_motion, mixamo_pipeline")
