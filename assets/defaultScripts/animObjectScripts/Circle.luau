local MathAnim = require("anim-core")
local AnimGui = require("anim-gui")
local Math = require("anim-math")

local radius = 3.0

-- This is automatically called when the object is generated
function generate(circle: MathAnim.AnimObject)
    local equidistantControls: number = (4.0 / 3.0) * math.tan(math.pi / 8.0) * radius
    local translation: Vec2 = {x = radius, y = radius}
    
    circle.svgObject:beginPath({x = -radius + translation.x, y = 0.0 + translation.y})
    circle.svgObject:cubicTo(
        Math.addVec2({x = -radius, y = equidistantControls}, translation),
        Math.addVec2({x = -equidistantControls, y = radius}, translation),
        Math.addVec2({x = 0, y = radius}, translation)
    )
    circle.svgObject:cubicTo(
        Math.addVec2({x = equidistantControls, y = radius}, translation),
        Math.addVec2({x = radius, y = equidistantControls}, translation),
        Math.addVec2({x = radius, y = 0}, translation)
    )
    circle.svgObject:cubicTo(
        Math.addVec2({x = radius, y = -equidistantControls}, translation),
        Math.addVec2({x = equidistantControls, y = -radius}, translation),
        Math.addVec2({x = 0, y = -radius}, translation)
    )
    circle.svgObject:cubicTo(
        Math.addVec2({x = -equidistantControls, y = -radius}, translation),
        Math.addVec2({x = -radius, y = -equidistantControls}, translation),
        Math.addVec2({x = -radius, y = 0}, translation)
    )
    circle.svgObject:closePath(false)
end

-- This is called every frame while an object with this script is selected in the Scene Hierarchy
function onInspector(parent: MathAnim.AnimObject)
    radius = AnimGui.dragNumber(parent, ": Radius", radius)
end

-- This is called once on project load
function register()
    AnimGui.register({ label = "Circle", mainMenu = "Shapes", header = "2D Shapes"})
end