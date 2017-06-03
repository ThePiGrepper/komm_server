-- komm_server.lua --

savefilename="save.t"

-- loads state if available, do nothing otherwise.
function komm_loadState()
  if file.open(savefilename,'r') then
    state = file.read();
--    print(crypto.toHex(state))
    file.close()
    komm.setState(state)
    if(state == komm.getState()) then
      return 0 -- OK
    else
      return 1 -- fail
    end
  else
    return 2 -- no file
  end
end

function komm_saveState()
  state = komm.getState();
  file.open(savefilename,'w')
  -- it could be added some code here to only write different bytes.
  file.write(state)
  file.close()
end
