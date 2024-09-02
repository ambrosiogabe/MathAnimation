
local MathAnim = require("anim-core")
local AnimGui = require("anim-gui")

local sideLength = 3.0

-- This is automatically called when the object is generated
function generate(parent: MathAnim.AnimObject)
    parent.svgObject:beginPath({x = -sideLength / 2, y = -sideLength / 2})
    parent.svgObject:lineTo({x = -sideLength / 2, y = sideLength / 2})
    parent.svgObject:lineTo({x = sideLength / 2, y = sideLength / 2})
    parent.svgObject:lineTo({x = sideLength / 2, y = -sideLength / 2})
    parent.svgObject:closePath(true)
end

-- This is called every frame while an object with this script is selected in the Scene Hierarchy
function onInspector(parent: MathAnim.AnimObject)
    sideLength = AnimGui.dragNumber(parent, ": Side Length", sideLength)
end

-- This is called once on project load
function register()
    AnimGui.register({ label = "Square", mainMenu = "Shapes", header = "2D Shapes"})
end