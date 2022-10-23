//----------------------------------------------------------------------------
// The confidential and proprietary information contained in this file may
// only be used by a person authorised under and to the extent permitted
// by a subsisting licensing agreement from ARM Limited or its affiliates.
//
//          (C) COPYRIGHT 2018 ARM Limited or its affiliates.
//                        ALL RIGHTS RESERVED
//
// This entire notice must be reproduced on all copies of this file
// and copies of this file may only be made by a person if such person is
// permitted to do so under the terms of a subsisting license agreementv
// from ARM Limited or its affiliates.
//----------------------------------------------------------------------------
const downloadTar = function(name, files) { 

    let blockSize = 512;

    const createBuffer = (size) => {
        let arr = new Uint8Array(new ArrayBuffer(size));
        for (var i = 0; i < arr.length; i++) {
            arr[i] = 0;
        }
        return arr;
    };

    const joinBuffers = (buffer1,buffer2 ) => {
        let tmp = new Uint8Array(buffer1.byteLength + buffer2.byteLength);
        tmp.set(new Uint8Array(buffer1), 0);
        tmp.set(new Uint8Array(buffer2), buffer1.byteLength);
        return tmp.buffer;
    };

    const fileBlockSize = (fileData) => fileData.byteLength ? Math.ceil(fileData.byteLength / blockSize) * blockSize: blockSize;

    const pad = (str, len) => (str.length >= len) ? str: str + new Array(len - str.length + 1).join("\x00");

    const createHeader = (file, fileData) => {
        let headerData =  {
            name : {offset: 0, size: 100, name: 'File name', value: file.name},
            mode: {offset: 100, size: 8, name: 'File mode', value: '0000644'},
            UID: {offset: 108, size: 8, name: 'UID', value: '0001000'},
            GUID: {offset: 116, size: 8, name: 'GUID', value: '0001000'},
            size: {offset: 124, size: 12, name: 'File size in bytes (octal base)', value: fileData.byteLength.toString(8)},
            date: {offset: 136, size: 12, name: 'Last modification time in numeric Unix time format (octal)', value: Math.round(new Date(file.lastModifiedDate).getTime() / 1000).toString(8)},
            checksum: {offset: 148, size: 8, name: 'Checksum for header record', value: '        '},
            type: {offset: 156, size: 1, name: 'Type flag', value: '0'},
            link: {offset: 157, size: 100, name: '(same field as in old format)', value: ''},
            ustar: {offset: 257, size: 6, name: 'UStar indicator "ustar" then NUL', value: 'ustar '},
            ustarVersion: {offset: 263, size: 2, name: 'UStar version', value: ''},
            ownerUserName: {offset: 265, size: 32, name: 'Owner user name', value: 'user'},
            ownerGroupName: {offset: 297, size: 32, name: 'Owner group name', value: 'user'}
        };
        let checksum = 0;
        for (let item in headerData) {
            if (headerData.hasOwnProperty(item)) {
                let info = headerData[item];
                for (let i = 0; i < info.value.length; i += 1) {
                    checksum += info.value.charCodeAt(i);
                }
            }
        }
        headerData.checksum.value = checksum.toString(8);
        let headerArray = createBuffer(blockSize);
        for (let item in headerData) {
            if (headerData.hasOwnProperty(item)) {
                let value = pad(headerData[item].value, headerData[item].size);
                let bufferPosition = headerData[item].offset;
                for (let i = 0; i < value.length; i++) {
                    headerArray[bufferPosition] = value.charCodeAt(i);
                    bufferPosition++;
                }
            }
        }
        return headerArray.buffer;
    };

    const reader = new FileReader();

    const readFile = (file) => new Promise((resolve, reject) => {
        reader.onload = () => {
            resolve(reader.result);
        };
        reader.onerror = (err) => {
            reject(err);
        };
        reader.readAsArrayBuffer(file);
    });

    const createFileSection = (file) => new Promise((resolve, reject) => {
        return readFile(file).then((result) => {
            let fileData = joinBuffers(result, new ArrayBuffer(fileBlockSize(result) - result.byteLength));
            let header = createHeader(file, fileData);
            let joinedBuffers = joinBuffers(header, fileData);
            resolve(joinedBuffers);
        }).catch((err) => {
            reject(err);
        });
    });

    function asyncQueue(makeGenerator){
        return function () {
            let generator = makeGenerator.apply(this, arguments);

            const handle = function(result){
                if (result.done) {
                    return Promise.resolve(result.value);
                }

                return Promise.resolve(result.value).then(function (res){
                    return handle(generator.next(res));
                }, function (err){
                    return handle(generator.throw(err));
                });
            };

            try {
                return handle(generator.next());
            } catch (ex) {
                return Promise.reject(ex);
            }
        };
    }

    let tarBuffer;

    let queue = asyncQueue(function*(filesList) {
        for (let i = 0; i < filesList.length; i++) {
            document.querySelector('#tar-progress').setAttribute('value', ((i + 1) / filesList.length).toString());
            let fileSection = yield createFileSection(filesList[i]);
            tarBuffer = tarBuffer? joinBuffers(tarBuffer, fileSection): fileSection;
        }
        yield;
    });
    return queue(files).then(() => {
        let dataURL = URL.createObjectURL(new File([tarBuffer], name));
        let a = document.createElement('a');
        let id = 'download-tar-hidden-link';
        a.id = id;
        a.textContent = name;
        a.href = dataURL;
        a.download = name;
        let elem = document.querySelector('#' + id);
        if (elem) {
            document.querySelector('.footer').removeChild(elem);
        }
        a.classList.add('hidden');
        document.querySelector('.footer').appendChild(a);
        document.querySelector('#tar-progress').classList.add('hidden');
        a.click();
    });
};
