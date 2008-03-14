function repeatability_demo

fprintf(1,'It may take a while, i.e. 30min \n');
%mex c_eoverlap.cxx;


% define the test cases.
i=1;
tests{i}.dir = 'bark';
tests{i}.number = 6;
tests{i}.title = 'Zoom + rot (bark)';
tests{i}.xlabel = 'scale change';
tests{i}.xrange = [2:6];

i=i+1;
tests{i}.dir = 'bikes';
tests{i}.number = 6;
tests{i}.title = 'Blur (bikes)';
tests{i}.xlabel = 'increasing blur';
tests{i}.xrange = [2:6];

i=i+1;
tests{i}.dir = 'boat';
tests{i}.number = 6;
tests{i}.title = 'Zoom + rot (boat)';
tests{i}.xlabel = 'scale change';
tests{i}.xrange = [1.1 1.38 1.9 2.35 2.8]; % approx factors from paper

i=i+1;
tests{i}.dir = 'graf';
tests{i}.number = 6;
tests{i}.title = 'Viewpoint (grafitti)';
tests{i}.xlabel = 'viewpoint angle';
tests{i}.xrange = [20:10:60];

i=i+1;
tests{i}.dir = 'leuven';
tests{i}.number = 6;
tests{i}.title = 'Exposure (leuven)';
tests{i}.xlabel = 'decreasing light';
tests{i}.xrange = [2:6];

i=i+1;
tests{i}.dir = 'trees';
tests{i}.number = 6;
tests{i}.title = 'Focus (trees)';
tests{i}.xlabel = 'increasing blur';
tests{i}.xrange = [2:6];

i=i+1;
tests{i}.dir = 'ubc';
tests{i}.number = 6;
tests{i}.title = 'JPEG comp (ubc)';
tests{i}.xlabel = 'JPEG compression (100 - quality) %';
tests{i}.xrange = [60 80 90 95 98];

i=i+1;
tests{i}.dir = 'wall';
tests{i}.number = 6;
tests{i}.title = 'Viewpoint (wall)';
tests{i}.xlabel = 'viewpoint angle';
tests{i}.xrange = [20:10:60];

tests_nb = i;

%det_suffix=['haraff';'hesaff';'mseraf';'ibraff';'ebraff';];

% keep order in sync with calles below!
det_suffix={'surf', 'mser', 'matchpoint', 'panomatic' };
det_nb=4;


for test=1:tests_nb,
  fprintf(1, '*** Running test %s\n', tests{test}.title);
  cdir = tests{test}.dir;
  for i=1:tests{test}.number
    fprintf(1, '\n*** Processing image %d\n', i);

    %detectFeatures(sprintf('./surf.ln -i %s/img%d.pgm -o %s/img%d.%s', cdir, i, cdir, i, det_suffix{1}));
    %detectFeatures(sprintf('./mser.ln -t 2 -es 2 -i %s/img%d.ppm -o %s/img%d.%s', cdir, i, cdir, i,det_suffix{2}));
    %detectFeatures(sprintf('./matchpoint -t %s/img%d.ppm %s/img%d.%s', cdir, i, cdir, i, det_suffix{3}));
    detectFeatures(sprintf('./panomatic --fullscale --onlykeypoints -o %s/img%d.%s %s/img%d.ppm', cdir, i, det_suffix{4}, cdir, i));

    %detectFeatures(sprintf('./h_affine.ln -haraff -sift -i img%d.ppm -o img%d.%s -thres 1000',i,i,det_suffix{1}));
    %detectFeatures(sprintf('./h_affine.ln -hesaff -sift -i img%d.ppm  -o img%d.%s -thres 500',i,i,det_suffix(2,:)));
    %detectFeatures(sprintf('./ibr.ln   img%d.ppm img%d.%s -scalefactor 1.0',i,i,det_suffix(4,:)));
    %detectFeatures(sprintf('./ebr.ln   img%d.ppm img%d.%s',i,i,det_suffix(5,:)));
  end

  figure(1);clf;
  grid on;
  ylabel('repeatebility %')
  xlabel('viewpoint angle');
  title(tests{test}.title);
  hold on;
  figure(2);clf;
  grid on;
  ylabel('nb of correspondences');
  xlabel('viewpoint angle');
  title(tests{test}.title);
  hold on;


  mark=['-kx';'-rv';'-gs';'-m+';'-bp'];
  for d=1:det_nb
    fprintf(1, '\n*** Analysing results for %s\n', det_suffix{d});
    %fflush(1);
    seqrepeat=[];
    seqcorresp=[];

    for i=2:tests{test}.number
      file1=sprintf('%s/img1.%s', cdir, det_suffix{d});
      file2=sprintf('%s/img%d.%s', cdir, i,det_suffix{d});
      Hom=sprintf('%s/H1to%dp',cdir,i);
      imf1=[cdir '/img1.ppm'];
      imf2=sprintf('%s/img%d.ppm',cdir,i);
      [erro,repeat,corresp, match_score,matches, twi]=repeatability(file1,file2,Hom,imf1,imf2, 1);
      save twi_repeat.txt twi

      seqrepeat=[seqrepeat repeat(4)];
      seqcorresp=[seqcorresp corresp(4)];

    end
    figure(1);  plot(tests{test}.xrange,seqrepeat,mark(d,:));
    figure(2);  plot(tests{test}.xrange,seqcorresp,mark(d,:));
  end

  %figure(1);legend(det_suffix(1,:),det_suffix(2,:),det_suffix(3,:),det_suffix(4,:),det_suffix(5,:));
  figure(1); legend(det_suffix);
  axis([ min(tests{test}.xrange) max(tests{test}.xrange) 0 100 ]);
  print ([tests{test}.dir '_repeatability.png'], '-dpng');
  print ([tests{test}.dir '_repeatability.eps'], '-depsc');
  print -dPNG repeatability.png;
  %figure(2);legend(det_suffix(1,:),det_suffix(2,:),det_suffix(3,:),det_suffix(4,:),det_suffix(5,:));
  figure(2); legend(det_suffix);
  print ([tests{test}.dir '_correspondences.png'], '-dpng');
  print ([tests{test}.dir '_correspondences.eps'], '-depsc');

end


function detectFeatures(command)
fprintf(1,'Detecting features: %s\n',command);
[status,result] = system(command);

