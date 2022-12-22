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