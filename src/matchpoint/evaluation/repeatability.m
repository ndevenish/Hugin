function [erro,repeat,corresp, match_score,matches, twi]=myrepeatability(file1,file2,Hom,imf1,imf2, common_part)
%
%
%Computes repeatability and overlap score between two lists of features
%detected in two images.
%   [erro,repeat,corresp,matched,matchedp]=repeatability('file1','file2','H4to1','imf1','imf2','-ko');
%
%IN: 
%    file1 - file 1 with detected features         
%    file2 - file 2 with detected features 
%    H - file with 3x3 Homography matrix from image 1 to 2, x2=H*x1
%        Assumes that image coordiantes are 0..width.
%    imf1 - image file  1
%    imf2 - image file  2
%    common_part - flag should be set to 1 for repeatability and 0 for descriptor performance
%
%OUT :    erro - overlap %
%         repeat - repeatability %
%         corresp - nb of correspondences
%         match_score  - matching score
%         matches - number of correct nearest neighbour matches
%         twi - matrix with overlap errors<50\%
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


fprintf(1,'Reading and sorting the regions from\n %s \n %s\n',file1,file2);

[f1 s1 dimdesc1]=loadFeatures(file1);
[f2 s2 dimdesc2]=loadFeatures(file2);

H=load(Hom);


fprintf(1,'nb of regions in file1 %d - descriptor dimension %d.\n',s1,dimdesc1);
fprintf(1,'nb of regions in file2 %d - descriptor dimension %d.\n',s2,dimdesc2);

isDescriptor=1;
if size(f1,1)==5 & size(f1,1)==size(f2,1) 
isDescriptor=0;
fprintf(1,'%s looks like file with affine regions...\n',file1);
  if  size(f1,1)~= 5 | size(f1,1) ~= 5
    error('Wrong ascii format of %s or %s files.',file1,file2);
  end
elseif dimdesc1>1 & dimdesc1==dimdesc2 
isDescriptor=1;
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
if common_part==1
fprintf(1,'Removing features from outside of the common image part...\n');
im1=imread(imf1);
im2=imread(imf2);
im1x=size(im1);
im1y=im1x(1);
im1x=im1x(2);
im2x=size(im2);
im2y=im2x(1);
im2x=im2x(2);
ind=find((feat1(:,1)+feat1(:,8))<im1x & (feat1(:,1)-feat1(:,8))>0 & (feat1(:,2)+feat1(:,9))<im1y & (feat1(:,2)-feat1(:,9))>0);
feat1=feat1(ind,:);
feat1t=feat1t(ind,:);
ind=find((feat1t(:,1)+feat1t(:,8))<im2x & (feat1t(:,1)-feat1t(:,8))>0 & (feat1t(:,2)+feat1t(:,9))<im2y & (feat1t(:,2)-feat1t(:,9))>0);
feat1=feat1(ind,:);
feat1t=feat1t(ind,:);
scales1=scales1(ind);

ind=find((feat2(:,1)+feat2(:,8))<im2x & (feat2(:,1)-feat2(:,8))>0 & (feat2(:,2)+feat2(:,9))<im2y & (feat2(:,2)-feat2(:,9))>0);
feat2t=feat2t(ind,:);
feat2=feat2(ind,:);
ind=find((feat2t(:,1)+feat2t(:,8))<im1x & (feat2t(:,1)-feat2t(:,8))>0 & (feat2t(:,2)+feat2t(:,9))<im1y & (feat2t(:,2)-feat2t(:,9))>0);
feat2t=feat2t(ind,:);
feat2=feat2(ind,:);
scales2=scales2(ind);

fprintf(1,'nb of regions in common part in image1 %d.\n',size(feat1,1));
fprintf(1,'nb of regions in common part in image2 %d.\n',size(feat2t,1));
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
end 

sf=min([size(feat1,1) size(feat2t,1)]);

feat1=feat1';
feat1t=feat1t';
feat2t=feat2t';
feat2=feat2';

fprintf(1,'Computing overlap error & selecting one-to-one correspondences: ');
tic;
%c_eoverlap is a C implementation to compute the overlap error.
[wout, twout, dout, tdout]=c_eoverlap(feat1,feat2t,common_part);
t=toc;
fprintf(1,' %.1f sec.\n',t);
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


erro=[10:10:60];
corresp=zeros(1,6);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%compute the number of correspondences
for i=1:6,
wi=(wout<erro(i));
corresp(i)=sum(sum(wi));
end
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
repeat=100*corresp/sf;


fprintf(1,'\noverlap error: ');
fprintf(1,'%.1f ',erro);
fprintf(1,'\nrepeatability: ');
fprintf(1,'%.1f ',repeat);
fprintf(1,'\nnb of correspondences: ');
fprintf(1,'%.0f ',corresp);
fprintf(1,'\n');

match_overlap=40;
if common_part==0
match_overlap=50;
end
match_score=0;
matches=0;
twi=[];
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
if isDescriptor==1
fprintf(1,'Matching with the descriptor for the overlap error < %d %%',match_overlap);
if common_part==0
twi=(twout<match_overlap);
else
twi=(wout<match_overlap);
end

dx=(dout<10000).*(twi);
matches=sum(sum(dx));
match_score=100*matches/sf;

fprintf(1,'\nMatching score  %0.1f, nb of correct matches %.1f.',match_score,matches);
end
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


%end
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
%end
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
%end


function [feat nb dim]=loadFeatures(file)
fid = fopen(file, 'r');
dim=fscanf(fid, '%f',1);
if dim==1
dim=0;
end
nb=fscanf(fid, '%d',1);
feat = fscanf(fid, '%f', [5+dim, inf]);
fclose(fid);
%end


