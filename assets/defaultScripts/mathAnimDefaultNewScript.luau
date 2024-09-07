local MathAnim = require("anim-core")

-- This is automatically called when the object is generated
function generate(parent: MathAnim.AnimObject)
    local waveform: MathAnim.AnimObject = MathAnim.createAnimObject(parent)
    waveform:setName("Waveform")
    waveform:setPositionVec({x = 0, y = 0, z = 0})
    waveform:setColor({r = 120, g = 207, b = 114, a = 255})

    local numSamples = 180
    local sampleWidth = 0.08
    local currentX = -1 * numSamples / 2 * sampleWidth

    local maxHeight = 1.5

    waveform.svgObject:beginPath({x = currentX, y = 0.0})
    for i=0,numSamples do 
        local randomHeight = math.random()
        currentX = currentX + sampleWidth 
        if i > 30 and i < 60 then 
            randomHeight *= 0.2
        end
        waveform.svgObject:lineTo({x = currentX, y = randomHeight * maxHeight * 0.5})
    end
    for i=numSamples,0,-1 do 
        local randomHeight = math.random()
        currentX = currentX - sampleWidth 
        if i > 30 and i < 60 then 
            randomHeight *= 0.2
        end
        waveform.svgObject:lineTo({x = currentX, y = -randomHeight * maxHeight})
    end
    waveform.svgObject:closePath(true)
end

-- @param t: The percent of time, ranging from 0.0-1.0, that this animation is at. 0.0
--           means the animation just started, 1.0 means the animation is complete.
-- @param obj: The object that's being animated
function onAnimate(t: number, obj: MathAnim.AnimObject)
    obj:setPercentCreated(t)
    
    local fadeInStart = 0.8
    local amountToFadeIn = ((t - fadeInStart) / (1.0 - fadeInStart))
    local percentToFadeIn = math.max(math.min(amountToFadeIn, 1.0), 0.0)

    local color = obj:getColor()
    color.a = percentToFadeIn * color.a 
    obj:setColor(currentColor)

    if (obj:getStrokeWidth() <= 0.0) then
        local strokeColor = obj:getStrokeColor()
        strokeColor.a = (1.0 - percentToFadeIn) * 255.0
        obj:setStrokeColor(strokeColor)
    end
end

function onInspector(parent: MathAnim.AnimObject)

end