function [cmatch_nn, tmatch_nn,cmatch_sim,tmatch_sim, corresp_sim, cmatch_rn,tmatch_rn]=descperf(file1,file2,Hom,imf1,imf2,corresp_nn,twi)
%
%
%Computes repeatability and overlap score between two lists of features
%detected in two images.
%   [erro,repeat,corresp,matched,matchedp]=repeatability('file1','file2','H4to1','imf1','imf2','-ko');
%
%IN: 
%    file1 - file 1 with feature parameters         
%    file2 - file 2 with feature parameters 
%    H - file with 3x3 Homography matrix from image 1 to 2, x2=H*x1
%        Assumes that image coordiantes are 0..width.
%    imf1 - image file  1
%    imf2 - image file  2
%    curv - color and mark of plot curv
%
%OUT :    erro - overlap %
%         repeat - repeatability %
%         corresp - nb of correspondences
%         matched - number of regions correctly matched with descriptors
%         matchedp - matched/corresp
%
%
%  region file format :
%--------------------
%descriptor_size  
%nbr_of_regions
%x1 y1 a1 b1 c1 d1 d2 d3 ...
%x2 y2 a2 b2 c2 d1 d2 d3 ...
%....
%....
%---------------------
%x, y - center coordinates
%a, b, c - ellipse parameters ax^2+2bxy+cy^2=1
%d1 d2 d3 ... - descriptor invariants
%if descriptor_size<=1 the descriptor is ignored

fprintf(1,'Reading and sorting the regions...\n');

[f1 s1 dimdesc1]=loadFeatures(file1);
[f2 s2 dimdesc2]=loadFeatures(file2);

H=load(Hom);


fprintf(1,'nb of regions in file1 %d - descriptor dimension %d.\n',s1,dimdesc1);
fprintf(1,'nb of regions in file2 %d - descriptor dimension %d.\n',s2,dimdesc2);


if size(f1,1)==5 & size(f1,1)==size(f2,1) 
fprintf(1,'%s looks like file with affine regions...\n',file1);
  if  size(f1,1)~= 5 | size(f1,1) ~= 5
    error('Wrong ascii format of %s or %s files.',file1,file2);
  end
elseif dimdesc1>1 & dimdesc1==dimdesc2 
  fprintf(1,'%s, %s look like files with descriptors...\n',file1,file2);
else
    error('Different descriptor dimension in %s or %s files.',file1,file2);
end

dimdesc=dimdesc1;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% allocate vectors for the features
feat1=zeros(9+dimdesc,s1);
feat2=zeros(9+dimdesc,s2);
feat1(1:5,1:s1)=f1(1:5,1:s1);
feat2(1:5,1:s2)=f2(1:5,1:s2);
if size(f1,1)>1
feat1(10:9+dimdesc,1:s1)=f1(6:5+dimdesc,1:s1);
feat2(10:9+dimdesc,1:s2)=f2(6:5+dimdesc,1:s2);
end
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%project regions from one image to the other
HI=H(:, 1:3);
H=inv(HI);
fprintf(1,'Projecting 1 to 2...');
[feat1 feat1t scales1]=project_regions(feat1',HI);
fprintf(1,'and 2 to 1...\n');
[feat2 feat2t scales2]=project_regions(feat2',H);

sf=min([size(feat1,1) size(feat2t,1)]);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

feat1=feat1';
feat1t=feat1t';
feat2t=feat2t';
feat2=feat2';


%rows nb are regions from image 1 and columns nb from image 2 
fprintf(1,'Computing descriptor distance & selecting one-to-one nearest neighbours: ');
tic;
[dout, tdout, sdout]=descdist(feat1,feat2t);

t=toc;
fprintf(1,' %.1f sec.\n',t);

tds=size(twi);
ds=size(dout);
save twi_descperf.txt dout tdout sdout;
if tds(1)~=ds(1) | tds(2)~=ds(2)
  error('Matrix twi computed with repeatability.m does not match the matrix computed from input files.\n');
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
fprintf(1,'\nNearest neighbour matching : ');
tic;
tmatch_nn=ones(1,20);
match_dist=zeros(size(tmatch_nn));
cmatch_nn=zeros(size(tmatch_nn));
dss=size(tmatch_nn,2);

% if the precision is in wrong range of values then tmatch_nn has to be changed.
tmatch_nn(1)=10;
for i=2:dss-4
tmatch_nn(i)=ceil(tmatch_nn(i-1)+40);
end
for i=dss-7:dss
tmatch_nn(i)=ceil(tmatch_nn(i-1)+200);
end


for i=1:dss
dx=(dout<tmatch_nn(i));
wx=dx.*twi;
match_dist(i)=max(max(wx.*tdout));
cmatch_nn(i)=sum(sum(wx));
end

t=toc;
fprintf(1,' %.1f sec.\n',t);

fprintf(1,'Nb of nn correspondences %d',corresp_nn);
fprintf(1,'\nnb total match nn: ');
fprintf(1,'%d ',tmatch_nn);
fprintf(1,'\nnb correct match nn: ');
fprintf(1,'%d ',cmatch_nn);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
fprintf(1,'\n\nSimilarity tmatching : ');

tic;
corresp_sim=sum(sum(twi));
for i=1:dss
dx=(tdout<match_dist(i));
tmatch_sim(i)=sum(sum(dx));
wx=dx.*twi;
cmatch_sim(i)=sum(sum(wx));
end
t=toc;
fprintf(1,'%.2f sec\n',t);
fprintf(1,'Nb of similarity correspondences %d.\n',corresp_sim);

recall=cmatch_sim/corresp_sim;
precision=(tmatch_sim-cmatch_sim)./tmatch_sim;

  fprintf(1,'ntotal match sim: ');
  fprintf(1,'%d ',tmatch_sim);
  fprintf(1,'\n');
  fprintf(1,'correct match sim: ');
  fprintf(1,'%d ',cmatch_sim);
  fprintf(1,'\n');
  fprintf(1,'precision sim %%: ');
  fprintf(1,'%0.3f ',precision);
  fprintf(1,'\n');
  fprintf(1,'recall sim %%: ');
  fprintf(1,'%0.3f ',recall);
  fprintf(1,'\n');

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

fprintf(1,'\n\nNN ratio matching : ');

tic;
ds=size(sdout);
rnmatch_dist=fliplr([1:.05:1.2,1.2:.3:2.8]);
cmatch_rn=zeros(size(rnmatch_dist));
tmatch_rn=zeros(size(rnmatch_dist));
dss=size(rnmatch_dist,2);


for i=1:dss
dx=(sdout>rnmatch_dist(i));
tmatch_rn(i)=sum(sum(dx));
wx=dx.*twi;
cmatch_rn(i)=sum(sum(wx));
end

t=toc;
fprintf(1,' %.2f sec.\n',t);


fprintf(1,'Nb of rn correspondences %d.\n',corresp_nn);
fprintf(1,'nb total match rn: ');
fprintf(1,'%d ',tmatch_rn);
fprintf(1,'\nnb correct match rn: ');
fprintf(1,'%d ',cmatch_rn);
fprintf(1,'\n');


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

end
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function [feat,featp,scales]=project_regions(feat,H)

s=size(feat);
s1=s(1);

featp=feat;
scales=zeros(1,s1);

for c1=1:s1,%%%%%%%%%%%%%%%%%
%feat(c1,3:5)=(1/25)*feat(c1,3:5);
Mi1=[feat(c1,3) feat(c1,4);feat(c1,4) feat(c1,5)];

%compute affine transformation
[v1 e1]=eig(Mi1);
d1=(1/sqrt(e1(1))); 
d2=(1/sqrt(e1(4))); 
sc1=sqrt(d1*d2);
feat(c1,6)=d1;
feat(c1,7)=d2; 
scales(c1)=sqrt(feat(c1,6)*feat(c1,7));

%bounding box
feat(c1,8) = sqrt(feat(c1,5)/(feat(c1,3)*feat(c1,5) - feat(c1,4)^2));
feat(c1,9) = sqrt(feat(c1,3)/(feat(c1,3)*feat(c1,5) - feat(c1,4)^2));


Aff=getAff(feat(c1,1),feat(c1,2),sc1, H);

%project to image 2
l1=[feat(c1,1),feat(c1,2),1];
l1_2=H*l1';
l1_2=l1_2/l1_2(3);
featp(c1,1)=l1_2(1);
featp(c1,2)=l1_2(2);
BMB=inv(Aff*inv(Mi1)*Aff');
[v1 e1]=eig(BMB);
featp(c1,6)=(1/sqrt(e1(1)));
featp(c1,7)=(1/sqrt(e1(4))); 
featp(c1,3:5)=[BMB(1) BMB(2) BMB(4)];
%bounding box in image 2
featp(c1,8) = sqrt(featp(c1,5)/(featp(c1,3)*featp(c1,5) - featp(c1,4)^2));
featp(c1,9) = sqrt(featp(c1,3)/(featp(c1,3)*featp(c1,5) - featp(c1,4)^2));
end
end
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function Aff=getAff(x,y,sc,H)
h11=H(1);
h12=H(4);
h13=H(7);
h21=H(2);
h22=H(5);
h23=H(8);
h31=H(3);
h32=H(6);
h33=H(9);
fxdx=h11/(h31*x + h32*y +h33) - (h11*x + h12*y +h13)*h31/(h31*x + h32*y +h33)^2;
fxdy=h12/(h31*x + h32*y +h33) - (h11*x + h12*y +h13)*h32/(h31*x + h32*y +h33)^2;

fydx=h21/(h31*x + h32*y +h33) - (h21*x + h22*y +h23)*h31/(h31*x + h32*y +h33)^2;
fydy=h22/(h31*x + h32*y +h33) - (h21*x + h22*y +h23)*h32/(h31*x + h32*y +h33)^2;

          Aff=[fxdx fxdy;fydx fydy];
end


function [feat nb dim]=loadFeatures(file)
fid = fopen(file, 'r');
dim=fscanf(fid, '%f',1);
if dim==1
dim=0;
end
nb=fscanf(fid, '%d',1);
feat = fscanf(fid, '%f', [5+dim, inf]);
fclose(fid);
end
