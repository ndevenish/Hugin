function descperf_demo

%fprintf(1,'It may take a while, i.e. 30min \n');
%mex c_eoverlap.cxx;


% define the test cases.
i=1;
tests{i}.dir = 'bark';
tests{i}.number = 6;
tests{i}.title = 'Zoom + rot (bark)';
tests{i}.xlabel = 'scale change';
tests{i}.xrange = [1:6];

i=i+1;
tests{i}.dir = 'bikes';
tests{i}.number = 6;
tests{i}.title = 'Blur (bikes)';
tests{i}.xlabel = 'increasing blur';
tests{i}.xrange = [1:6];

i=i+1;
tests{i}.dir = 'boat';
tests{i}.number = 6;
tests{i}.title = 'Zoom + rot (boat)';
tests{i}.xlabel = 'scale change';
tests{i}.xrange = [0 1.1 1.38 1.9 2.35 2.8]; % approx factors from paper

i=i+1;
tests{i}.dir = 'graf';
tests{i}.number = 6;
tests{i}.title = 'Viewpoint (grafitti)';
tests{i}.xlabel = 'viewpoint angle';
tests{i}.xrange = [10:10:60];

i=i+1;
tests{i}.dir = 'leuven';
tests{i}.number = 6;
tests{i}.title = 'Exposure (leuven)';
tests{i}.xlabel = 'decreasing light';
tests{i}.xrange = [2 2:6];

i=i+1;
tests{i}.dir = 'trees';
tests{i}.number = 6;
tests{i}.title = 'Focus (trees)';
tests{i}.xlabel = 'increasing blur';
tests{i}.xrange = [2 2:6];

i=i+1;
tests{i}.dir = 'ubc';
tests{i}.number = 6;
tests{i}.title = 'JPEG comp (ubc)';
tests{i}.xlabel = 'JPEG compression (100 - quality) %';
tests{i}.xrange = [60 60 80 90 95 98];

i=i+1;
tests{i}.dir = 'wall';
tests{i}.number = 6;
tests{i}.title = 'Viewpoint (wall)';
tests{i}.xlabel = 'viewpoint angle';
tests{i}.xrange = [10:10:60];

tests_nb = i;

%det_suffix=['haraff';'hesaff';'mseraf';'ibraff';'ebraff';];

% keep order in sync with calles below!
det_suffix={'matchpoint', 'surf'} % 'mser'} % , 'haraff', 'matchpoint' };
det_nb=2;


for test=1:tests_nb,
  fprintf(1, '*** Running test %s\n', tests{test}.title);
  cdir = tests{test}.dir;
  for i=1:tests{test}.number
    fprintf(1, '\n*** Processing image %d\n', i);

    detectFeatures(sprintf('matchpoint -t %s/img%d.ppm %s/img%d.%s', cdir, i, cdir, i, det_suffix{1}));
    detectFeatures(sprintf('./surf.ln -i %s/img%d.pgm -o %s/img%d.%s', cdir, i, cdir, i, det_suffix{2}));
    %detectFeatures(sprintf('./mser.ln -t 2 -es 2 -i %s/img%d.ppm -o %s/img%d.%s', cdir, i, cdir, i,det_suffix{2}));

    %detectFeatures(sprintf('./h_affine.ln -haraff -sift -i img%d.ppm -o img%d.%s -thres 1000',i,i,det_suffix{1}));
    %detectFeatures(sprintf('./h_affine.ln -hesaff -sift -i img%d.ppm  -o img%d.%s -thres 500',i,i,det_suffix(2,:)));
    %detectFeatures(sprintf('./ibr.ln   img%d.ppm img%d.%s -scalefactor 1.0',i,i,det_suffix(4,:)));
    %detectFeatures(sprintf('./ebr.ln   img%d.ppm img%d.%s',i,i,det_suffix(5,:)));
  end

  mark=['-kx';'-rv';'-gs';'-m+';'-bp'];
  for i=2:tests{test}.number
    fprintf(1, '\n*** Analysing results for test %s, number %d\n', tests{test}.title, i);

    figure(1);clf;
    grid on;
    xlabel('1-precision');
    ylabel('# correct matches / # total correspondences');
    title(sprintf('NN matching, %s, %s: %d', tests{test}.title, tests{test}.xlabel, tests{test}.xrange(i)));
    hold on;

    figure(2);clf;
    grid on;
    xlabel('1-precision');
    ylabel('# correct matches / # total correspondences');
    title(sprintf('Ratio matching, %s, %s: %d', tests{test}.title, tests{test}.xlabel, tests{test}.xrange(i)));
    hold on;

    figure(3);clf;
    grid on;
    xlabel('1-precision');
    ylabel('# correct matches / # total correspondences');
    title(sprintf('Similarity matching, %s, %s: %d', tests{test}.title, tests{test}.xlabel, tests{test}.xrange(i)));
    hold on;

    legendnames = {};
    %fflush(1);
    for d=1:det_nb
      file1=sprintf('%s/img1.%s', cdir, det_suffix{d});
      file2=sprintf('%s/img%d.%s', cdir, i,det_suffix{d});
      Hom=sprintf('%s/H1to%dp',cdir,i);
      imf1=[cdir '/img1.ppm'];
      imf2=sprintf('%s/img%d.ppm',cdir,i);
      [erro,repeat,corresp, match_score,matches, twi]=repeatability(file1,file2,Hom,imf1,imf2, 0);
      save twi_repeat.txt twi

      corresp_nn = corresp(5);
      [cmatch_nn, tmatch_nn,cmatch_sim,tmatch_sim, corresp_sim, cmatch_rn,tmatch_rn] = descperf(file1, file2, Hom, imf1, imf2, corresp_nn,twi)

      legendnames{d} = sprintf('%s, %d matches', det_suffix{d}, corresp_nn);
      recall_nn = cmatch_sim / corresp_nn;
      precision_nn = (tmatch_nn-cmatch_nn)./tmatch_nn;
      figure(1);  plot(precision_nn, recall_nn, mark(d,:));

      recall_rn = cmatch_rn / corresp_nn;
      precision_rn = (tmatch_rn-cmatch_rn)./tmatch_rn;
      figure(2);  plot(precision_rn, recall_rn, mark(d,:));

      recall_sim = cmatch_sim / corresp_sim;
      precision_sim = (tmatch_sim-cmatch_sim)./tmatch_sim;
      figure(3);  plot(precision_sim, recall_sim, mark(d,:));

    end
    figure(1); legend(legendnames);
    axis ([0 1.4 0 1]);
    print ([tests{test}.dir '_recall_nn.png'], '-dpng');
    print ([tests{test}.dir '_recall_nn.eps'], '-depsc');

    figure(2); legend(legendnames);
    axis ([0 1.4 0 1]);
    print ([tests{test}.dir '_recall_rn.png'], '-dpng');
    print ([tests{test}.dir '_recall_rn.eps'], '-depsc');

    figure(3); legend(legendnames);
    axis ([0 1.4 0 1]);
    print ([tests{test}.dir '_recall_sim.png'], '-dpng');
    print ([tests{test}.dir '_recall_sim.eps'], '-depsc');
  end
end


function detectFeatures(command)
fprintf(1,'Detecting features: %s\n',command);
[status,result] = system(command);

