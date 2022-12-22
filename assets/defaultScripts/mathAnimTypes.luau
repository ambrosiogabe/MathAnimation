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
    setName: (name: string) -> (),
    setPositionVec: (position: Vec3) -> (),
    setPosition: (x: number, y: number, z: number) -> (),
    svgObject: SvgObject,
}

export type SvgObject = {
    beginPath: (startPosition: Vec2) -> (),
    closePath: (connectLastPoint: boolean) -> (),
    setPathAsHole: () -> (),

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
    arcToRel: (radius: Vec2, xAxisRot: number, largeArcFlag: boolean, sweepFlag: boolean, p0: Vec2) -> ()
}

type MathAnimModule = {
    createAnimObject: (parent: AnimObject) -> AnimObject
}

local MathAnim: MathAnimModule
return MathAnim