#include "scripting/MathAnimGlobals.h"

namespace MathAnim
{
	namespace MathAnimGlobals
	{
        static const std::string builtinDefinitionLuaSrc = R"BUILTIN_SRC(
type Logger = {
    write: <T...>(T...) -> (),
    info: <T...>(T...) -> (),
    warn: <T...>(T...) -> (),
    error: <T...>(T...) -> ()
}

declare logger: Logger
)BUILTIN_SRC";

        static const std::string mathAnimModule = R"BUILTIN_TYPES(
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

export type AnimObject = {
    id: u64,
    setName: (self: AnimObject, name: string) -> (),
    setPositionVec: (self: AnimObject, position: Vec3) -> (),
    setPosition: (self: AnimObject, x: number, y: number, z: number) -> (),
    svgObject: SvgObject,
}

export type SvgObject = {
    beginPath: (startPosition: Vec2) -> (),

    -- Absolute svg commands
    moveTo: (position: Vec2) -> (),
    lineTo: (p0: Vec2) -> (),
    vtLineTo: (y0: number) -> (),
    hzLineTo: (x0: number) -> (),
    quadTo: (p0: Vec2, p1: Vec2) -> (),
    cubicTo: (p0: Vec2, p1: Vec2, p2: Vec2) -> (),
    arcTo: (radius: Vec2, xAxisRot: number, largeArcFlag: boolean, sweepFlag: boolean, p0: Vec2) -> (),

    -- Relative svg commands
    moveToRel: (position: Vec2) -> (),
    vtLineToRel: (y0: number) -> (),
    hzLineToRel: (x0: number) -> (),
    lineToRel: (p0: Vec2) -> (),
    quadToRel: (p0: Vec2, p1: Vec2) -> (),
    cubicToRel: (p0: Vec2, p1: Vec2, p2: Vec2) -> (),
    arcToRel: (radius: Vec2, xAxisRot: number, largeArcFlag: boolean, sweepFlag: boolean, p0: Vec2) -> (),

    -- Connects last point in path to start point
    closePath: () -> ()
}

type MathAnimModule = {
    createAnimObject: (parent: AnimObject) -> AnimObject
}

local MathAnim: MathAnimModule
return MathAnim
)BUILTIN_TYPES";

        std::string getMathAnimModule()
        {
            return mathAnimModule;
        }

        std::string getBuiltinDefinitionSource()
        {
            return builtinDefinitionLuaSrc;
        }

        std::string getMathAnimApiTypes()
        {
            return mathAnimModule;
        }
	}
}