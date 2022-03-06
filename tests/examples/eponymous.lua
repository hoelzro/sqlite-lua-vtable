local counter10 = require 'tests.examples.counter10'

local eponymous = {}

for k, v in pairs(counter10) do
  eponymous[k] = v
end

eponymous.connect = eponymous.create

return eponymous
