wkDir = 'D:/WkDir/DetectionProposals/VOC2007/Annotations/';
xml2yaml(wkDir);
wkDir = 'D:\WkDir\DetectionProposals\ImageNet\ILSVRC2012_bbox_val_v3\val\';
xml2yaml(wkDir);

ImgNetDir = 'D:\WkDir\DetectionProposals\ImageNet\ILSVRC2012_bbox_train_v2\';
d = dir(ImgNetDir);
isub = [d(:).isdir]; %# returns logical vector
nameFolds = {d(isub).name}';
nameFolds(ismember(nameFolds,{'.','..'})) = [];
for i=1:length(nameFolds)
    wkDir = [ImgNetDir nameFolds{i} '\'];
    fprintf('%d/%d: %s\n', i, length(nameFolds), wkDir);
    xml2yaml(wkDir);
end


