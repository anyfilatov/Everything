clear;
clc;

border(1) = {@(x) sqrt(25-(x-4).^2+10)};
border(2) = {@(x) sin(x)};

%x = [zeros(1,44)        0:0.1:5         zeros(1,68)+5        5:-0.1:0        ];
%y = [0:0.1:4.3     border{1}(0:0.1:5)   5.8:-0.1:-0.9    border{2}(5:-0.1:0)  ];

x = [zeros(1,21)        0:0.1:1           zeros(1,11)+1        1:0.1:4        zeros(1,21)+4       4:-0.1:2       zeros(1,21)+2       2:-0.1:1        zeros(1,6)+1      1:-0.1:0.5     zeros(1,6)+0.5      0.5:-0.1:0];
y = [0:0.1:2          zeros(1,11)+2         2:0.1:3           zeros(1,31)+3     3:-0.1:1        zeros(1,21)+1       1:-0.1:-1      zeros(1,11)-1      -1:0.1:-0.5    zeros(1,6)-0.5      -0.5:0.1:0      zeros(1,6) ];

threshold = 0.3;

pointsAmount = length(x);
khords = zeros(1, pointsAmount);
for i = 1 : pointsAmount
    distanceToKhodr = 0;
    j=0;
    while (distanceToKhodr < threshold)
        j = j+1;
        distanceToKhordOld = distanceToKhodr;
        distanceToKhodr = distance3(x(i),y(i),...
                           x(myMod(i+j,pointsAmount)),y(myMod(i+j,pointsAmount)),...
                           x(myMod(i-j,pointsAmount)),y(myMod(i-j,pointsAmount)));
    end;
    khords(i) = distance2(x(myMod(i-j+1,pointsAmount)),y(myMod(i-j+1,pointsAmount)),...
                         x(myMod(i+j-1,pointsAmount)),y(myMod(i+j-1,pointsAmount)));
end;


maxes=imregionalmax(khords);
mins = imregionalmin(khords);
index1 = -1;
index2 = -1;
for i = 1 : pointsAmount
    if maxes(i) == 1
        if index1 == -1
            index1 = i;
        else
            index1 = [index1 i];
        end;
    end;
    if mins(i) == 1
        if index2 == -1
            index2 = i;
        else
            index2 = [index2 i];
        end;
    end;
end;
figure; plot(x,y,'bx', x(index1),y(index1),'ro',x(index2),y(index2),'go', 'linewidth', 2); axis([-0.1,4.1,-1.1,3.1]);
ind = 1:pointsAmount;
figure; plot(ind,khords,'b',ind(index1),khords(index1),'ro',ind(index2),khords(index2),'go'); grid;