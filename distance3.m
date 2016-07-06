function [ res ] = distance3( x1,y1, x2,y2,x3,y3)
%distance3 calculates distanse from point p1(x1,y1) to line contains
%p2(x2,y2) and p3(x3,y3)
    
    res = abs((y3-y2).*x1+(x2-x3).*y1+x3.*y2-x2.*y3)/sqrt((y3-y2).^2+(x2-x3).^2);

end

