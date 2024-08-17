local MathAnim = require("math-anim")

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

-- TODO: Implement this
function onInspector(parent: MathAnim.AnimObject)

end