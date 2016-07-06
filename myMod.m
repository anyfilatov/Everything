function res = myMod(number,mod)
if number < 0
    number = -number;
    res = mod-rem(number,mod);
else
    res = rem(number,mod);
end;
if res == 0
    res = mod;
end;

end

