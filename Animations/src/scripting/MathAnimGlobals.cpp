#include "scripting/MathAnimGlobals.h"

namespace MathAnim
{
	namespace MathAnimGlobals
	{
        static const std::string_view builtinDefinitionLuaSrc = R"BUILTIN_SRC(
export type Logger = {
    write: <T...>(T...) -> (),
    info: <T...>(T...) -> (),
    warn: <T...>(T...) -> (),
    error: <T...>(T...) -> ()
}

declare logger: Logger

export type Vec2 = {
    x: number,
    y: number
}

export type Vec3 = {
    x: number,
    y: number,
    z: number
}

export type Vec4Number = {
    x: number,
    y: number,
    z: number,
    w: number
}

export type Vec4Color = {
    r: number,
    g: number,
    b: number,
    a: number
}

export type Vec4 = Vec4Number | Vec4Color

export type u64 = {
    low: number,
    hight: number
}
)BUILTIN_SRC";

        static constexpr std::string_view animCoreModule = R"BUILTIN_TYPES(
export type SvgObject = {
    ptr: any,
    objId: u64,

    beginPath: (svgObject: SvgObject, startPosition: Vec2) -> (),
    closePath: (svgObject: SvgObject, connectLastPoint: boolean) -> (),
    setPathAsHole: (svgObject: SvgObject) -> (),

    -- Absolute svg commands
    moveTo: (svgObject: SvgObject, position: Vec2) -> (),
    lineTo: (svgObject: SvgObject, p0: Vec2) -> (),
    vtLineTo: (svgObject: SvgObject, y0: number) -> (),
    hzLineTo: (svgObject: SvgObject, x0: number) -> (),
    quadTo: (svgObject: SvgObject, p0: Vec2, p1: Vec2) -> (),
    cubicTo: (svgObject: SvgObject, p0: Vec2, p1: Vec2, p2: Vec2) -> (),
    arcTo: (svgObject: SvgObject, radius: Vec2, xAxisRot: number, largeArcFlag: boolean, sweepFlag: boolean, p0: Vec2) -> (),

    -- Relative svg commands
    moveToRel: (svgObject: SvgObject, position: Vec2) -> (),
    vtLineToRel: (svgObject: SvgObject, y0: number) -> (),
    hzLineToRel: (svgObject: SvgObject, x0: number) -> (),
    lineToRel: (svgObject: SvgObject, p0: Vec2) -> (),
    quadToRel: (svgObject: SvgObject, p0: Vec2, p1: Vec2) -> (),
    cubicToRel: (svgObject: SvgObject, p0: Vec2, p1: Vec2, p2: Vec2) -> (),
    arcToRel: (svgObject: SvgObject, radius: Vec2, xAxisRot: number, largeArcFlag: boolean, sweepFlag: boolean, p0: Vec2) -> ()
}

export type AnimObject = {
    id: u64,
    setName: (self: AnimObject, name: string) -> (),
    setPositionVec: (self: AnimObject, position: Vec3) -> (),
    setPosition: (self: AnimObject, x: number, y: number, z: number) -> (),
    setColor: (self: AnimObject, color: Vec4) -> (),
    svgObject: SvgObject,
}

export type MathAnimModule = {
    createAnimObject: (parent: AnimObject) -> AnimObject
}

local MathAnim: MathAnimModule
return MathAnim
)BUILTIN_TYPES";

        static constexpr std::string_view animGuiModule = R"BUILTIN_TYPES(
local MathAnim = require('anim-core')

export type RegisterProps = {
    label: string,
    mainMenu: string?,
    header: string?
}

export type MathAnimGuiModule = {
    dragNumber: (parent: MathAnim.AnimObject, label: string, default: number?) -> number,
    colorPicker: (parent: MathAnim.AnimObject, label: string, default: Vec4Color?) -> Vec4Color,

    register: (props: RegisterProps) -> ()
}

local MathAnimGui: MathAnimGuiModule
return MathAnimGui
)BUILTIN_TYPES";

        static constexpr std::string_view animMathModule = R"BUILTIN_TYPES(
export type MathModule = {
    addVec2: (v1: Vec2, v2: Vec2) -> Vec2,
    addVec3: (v1: Vec3, v2: Vec3) -> Vec3,
    addVec4: (v1: Vec4, v2: Vec4) -> Vec4
}

function addVec2(v1: Vec2, v2: Vec2): Vec2
    return {x = v1.x + v2.x, y = v1.y + v2.y}
end

function addVec3(v1: Vec3, v2: Vec3): Vec3
    return {x = v1.x + v2.x, y = v1.y + v2.y, z = v1.z + v2.z}
end

function addVec4(v1Untyped: Vec4, v2Untyped: Vec4): Vec4
    local v1Any = v1Untyped :: any
    local v2Any = v2Untyped :: any

    if (v1Any["x"] and v2Any["x"]) then
        local v1 = v1Any :: Vec4Number
        local v2 = v2Any :: Vec4Number
        return {x = v1.x + v2.x, y = v1.y + v2.y, z = v1.z + v2.z, w = v1.w + v2.w}
    end

    if (v1Any["r"] and v2Any["r"]) then
        local v1 = v1Any :: Vec4Color
        local v2 = v2Any :: Vec4Color
        return {r = v1.r + v2.r, g = v1.r + v2.r, b = v1.b + v2.b, a = v1.a + v2.a}
    end

    if (v1Any["r"] and v2Any["x"]) then
         local v1 = v1Any :: Vec4Color
         local v2 = v2Any :: Vec4Number
         return {r = v1.r + v2.x, g = v1.g + v2.y, b = v1.b + v2.z, a = v1.a + v2.w}
    end

    local v1 = v1Any :: Vec4Number
    local v2 = v2Any :: Vec4Color
    return {x = v1.x + v2.r, y = v1.y + v2.g, z = v1.z + v2.b, w = v1.w + v2.a}
end


local Math: MathModule = {
   addVec2 = addVec2,
   addVec3 = addVec3,
   addVec4 = addVec4
}
return Math;
)BUILTIN_TYPES";

        std::string_view getAnimGuiModule()
        {
            return animGuiModule;
        }

        std::string_view getAnimCoreModule()
        {
            return animCoreModule;
        }

        std::string_view getAnimMathModule()
        {
            return animMathModule;
        }

        std::string_view getBuiltinDefinitionSource()
        {
            return builtinDefinitionLuaSrc;
        }
	}
}