local MathAnim = require("anim-core")
local AnimGui = require("anim-gui")

local stemWidth = 0.12
local stemLength = 1.8
local tipLength = 0.4
local tipWidth = 0.4

-- This is automatically called when the object is generated
function generate(parent: MathAnim.AnimObject)
    local halfLength = stemLength / 2
    local halfWidth = stemWidth / 2
    local tipOffset = tipWidth - stemWidth
    local halfTipOffset = tipOffset / 2
    local halfTipWidth = tipWidth / 2

    parent.svgObject:beginPath({x = -halfLength, y = -halfWidth})
    parent.svgObject:lineToRel({x = 0, y = stemWidth})
    parent.svgObject:lineToRel({x = stemLength, y = 0})
    parent.svgObject:lineToRel({x = 0, y = halfTipOffset})
    parent.svgObject:lineToRel({x = tipLength, y = -halfTipWidth})
    parent.svgObject:lineToRel({x = -tipLength, y = -halfTipWidth})
    parent.svgObject:lineToRel({x = 0, y = halfTipOffset})
    parent.svgObject:lineToRel({x = -stemLength, y = 0})
    parent.svgObject:closePath(true)
end

-- This is called every frame while an object with this script is selected in the Scene Hierarchy
function onInspector(parent: MathAnim.AnimObject)
    stemLength = AnimGui.dragNumber(parent, ": Stem Length##ArrowShape", stemLength)
    stemWidth = AnimGui.dragNumber(parent, ": Stem Width##ArrowShape", stemWidth)
    tipLength = AnimGui.dragNumber(parent, ": Tip Length##ArrowShape", tipLength)
    tipWidth = AnimGui.dragNumber(parent, ": Tip Width##ArrowShape", tipWidth)
end

-- This is called once on project load
function register()
    AnimGui.register({ label = "Arrow", mainMenu = "Shapes", header = "2D Shapes"})
end