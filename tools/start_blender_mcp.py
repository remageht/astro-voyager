"""
start_blender_mcp.py
====================
Helper script to launch Blender 5.2 and automatically start the MCP for Blender
server on localhost:9876.
Usage:
    & "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --python tools/start_blender_mcp.py
"""

import bpy
import time

def start_mcp():
    print("[start_blender_mcp] Initializing MCP for Blender...")
    # Ensure addon is enabled
    if not hasattr(bpy.ops, "blendermcp"):
        try:
            bpy.ops.preferences.addon_enable(module="blender_mcp")
            print("[start_blender_mcp] Enabled blender_mcp addon.")
        except Exception as e:
            print(f"[start_blender_mcp] Failed to enable addon: {e}")

    # Start server
    try:
        bpy.ops.blendermcp.start_server()
        print("[start_blender_mcp] MCP for Blender server is now listening on localhost:9876!")
    except Exception as e:
        print(f"[start_blender_mcp] Could not start MCP server: {e}")

# Register as timer callback to execute after UI initialization
bpy.app.timers.register(start_mcp, first_interval=0.5)
