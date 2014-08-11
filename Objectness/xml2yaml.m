%% Convert the file type of opencv xml annotations to yaml thus it can be read by opencv 
% This functions relies on http://code.google.com/p/yamlmatlab/
% The results needs to be further refined to deal with indentation problem

function xml2yaml(wkDir)
    fNs = dir([wkDir  '*.xml']);
    fNum = length(fNs);
    for i = 1:fNum
        [~, nameNE, ~] = fileparts(fNs(i).name); 
        %fprintf('%d/%d: %s\n', i, fNum, [wkDir nameNE]);
        fPathN = [wkDir nameNE '.yaml'];
        x=VOCreadxml([wkDir nameNE '.xml']);
        if isfield(x.annotation, 'owner')
            x.annotation = rmfield(x.annotation, 'owner');
        end        
        names = fieldnames(x.annotation.object);
        if (strcmp(names{end}, 'bndbox'))
            x.annotation.object = orderfields(x.annotation.object,  names([end, 1:end-1]));
        end
        WriteYaml(fPathN, x);
    end
end