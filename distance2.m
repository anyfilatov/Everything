function [ res ] = distance2( x1,y1,x2,y2)
%distance Calculates distance between point p1(x1,y1) and p2(x2,y2)

    res = sqrt((x2-x1).^2+(y2-y1).^2);
end

