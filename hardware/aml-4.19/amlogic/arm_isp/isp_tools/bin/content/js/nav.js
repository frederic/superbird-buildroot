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
let fixed = {
    component: null,
    connection: null
};

document.addEventListener('mouseover', function(event) {
    var componentID;
    if (event.target.classList.contains('connection-hover')) {
        var connectionID = event.target.getAttribute('data-connection-id');
        Array.from(document.querySelectorAll('.connection-highlight[data-connection-id="' + connectionID  +'"]')).forEach(function(item) {
            if (item.classList.contains('moving-connection-highlight')) {
                item.setAttribute('opacity', '0.6');
                item.setAttribute('visible', 'true');
            }
            else {
                item.setAttribute('opacity', '1');
                item.setAttribute('visible', 'true');
            }
        });
        componentID = event.target.getAttribute('data-component-id');
        let inputIndex = event.target.getAttribute('data-input-index');
        if (inputIndex > -1) {
            document.querySelector('.component-input-label[data-component-id="' + componentID  +'"]').setAttribute('opacity', '1');
            document.querySelector('.component-input-label-text[data-component-id="' + componentID + '"]').setAttribute('opacity', '1');
            document.querySelector('.component-input-label-text[data-component-id="' + componentID + '"]').childNodes[0].textContent = inputIndex;
        }
    }
    if (event.target.classList.contains('component')) {
        componentID = event.target.getAttribute('data-component-id');
        Array.from(document.querySelectorAll('.connection-highlight[data-component-id="' + componentID  +'"]')).forEach(function(item) {
            if (item.classList.contains('moving-connection-highlight')) {
                item.setAttribute('opacity', '0.6');
                item.setAttribute('visible', 'true');
            }
            else {
                item.setAttribute('opacity', '1');
                item.setAttribute('visible', 'true');
            }
        });
    }
});

document.addEventListener('mouseout', function(event) {
    var componentID;
    if (event.target.classList.contains('connection-hover')) {
        var connectionID = event.target.getAttribute('data-connection-id');
        componentID = event.target.getAttribute('data-component-id');
        if (connectionID !== fixed.connection) {
            Array.from(document.querySelectorAll('.connection-highlight[data-connection-id="' + connectionID + '"]')).forEach(function (item) {
                item.setAttribute('opacity', '0');
                item.removeAttribute('visible');
            });
        }
        if (componentID !== fixed.component) {
            Array.from(document.querySelectorAll('.component-input-label[data-component-id="' + componentID + '"]')).forEach(function (item) {
                item.setAttribute('opacity', '0');
                item.removeAttribute('visible');
            });

            Array.from(document.querySelectorAll('.component-input-label-text[data-component-id="' + componentID + '"]')).forEach(function (item) {
                item.setAttribute('opacity', '0');
                item.removeAttribute('visible');
            });
        }
    }
    if (event.target.classList.contains('component')) {
        componentID = event.target.getAttribute('data-component-id');
        Array.from(document.querySelectorAll('.connection-highlight[data-component-id="' + componentID + '"]')).forEach(function (item) {
            item.setAttribute('opacity', '0');
            item.removeAttribute('visible');
        });
    }
});

document.addEventListener('click', function(event) {

    if (event.button !== 0) { 
        return;
    }

    if (fixed.connection !== null) {
        Array.from(document.querySelectorAll('.connection-highlight[data-connection-id="' + fixed.connection  +'"]')).forEach(function(item) {
                item.setAttribute('opacity', '0');
                item.removeAttribute('visible');
        });
    }

    if (fixed.component !== null) {
        Array.from(document.querySelectorAll('.connection-highlight[data-component-id="' + fixed.component  +'"]')).forEach(function(item) {
                item.setAttribute('opacity', '0');
                item.removeAttribute('visible');
        });
        document.querySelector('.component-input-label[data-component-id="' + fixed.component  +'"]').setAttribute('opacity', '0');
        document.querySelector('.component-input-label-text[data-component-id="' + fixed.component + '"]').setAttribute('opacity', '0');
    }

    var componentID;
    fixed = {
        component: null,
        connection: null
    };
    if (event.target.classList.contains('connection-hover')) {
        var connectionID = event.target.getAttribute('data-connection-id');
        fixed['connection'] = connectionID;
        Array.from(document.querySelectorAll('.connection-highlight[data-connection-id="' + connectionID  +'"]')).forEach(function(item) {
            if (item.classList.contains('moving-connection-highlight')) {
                item.setAttribute('opacity', '0.6');
                item.setAttribute('visible', 'true');
            }
            else {
                item.setAttribute('opacity', '1');
                item.setAttribute('visible', 'true');
            }
        });
        componentID = event.target.getAttribute('data-component-id');
        let inputIndex = event.target.getAttribute('data-input-index');
        if (inputIndex > -1) {
            document.querySelector('.component-input-label[data-component-id="' + componentID  +'"]').setAttribute('opacity', '1');
            document.querySelector('.component-input-label-text[data-component-id="' + componentID + '"]').setAttribute('opacity', '1');
            document.querySelector('.component-input-label-text[data-component-id="' + componentID + '"]').childNodes[0].textContent = inputIndex;
        }
    }
    if (event.target.classList.contains('component')) {
        componentID = event.target.getAttribute('data-component-id');
        fixed['component'] = componentID;
    }
});

function scaleSVGViewBy(scale) {
    let dragSVG = document.getElementById('dragSVG');
    let masterSVG = document.getElementById('masterSVG');
    dragSVG.setAttributeNS(null, 'transform', 'scale(' + scale + ')');
    dragSVG.setAttribute('data-current-scale', scale);
    let dragRect = dragSVG.getBBox();
    masterSVG.setAttribute('width', ((dragRect.width * scale) + (dragRect.x * scale)).toString());
    masterSVG.setAttribute('height', ((dragRect.height * scale) + (dragRect.y * scale)).toString());
}


document.oninput = function(event) {
    if (event.target.id === 'zoom') {
        scaleSVGViewBy(+event.target.value);
    }
};

const draw = function() {
    document.getElementById('loading-text').innerHTML = 'Loading...';
    document.getElementById('loading-screen').classList.remove('hidden');
    let fileInput = document.getElementById('file-upload');
    let file = fileInput.files[0];
    let textType = /text.*/;
    if (file.type.match(textType)) {
        let options = {
            lineCost: 1,
            distanceFactor: 1,
        };

        let mainRoot = document.querySelector('#main-root:not(.hidden)');
        if (mainRoot) {
            if (mainRoot.value !== 'auto') {
                options.mainRoot = mainRoot.value;
            }
        }

        ISP(file, options).then(function() {
            scaleSVGViewBy(0.3);
            document.getElementById('zoom').value = 0.3;

            let mainRootList = document.getElementById('main-root');
            while (mainRootList.firstChild) {
                mainRootList.removeChild(mainRootList.firstChild);
            }
            let option = document.createElement('option');
            option.setAttribute('value', 'auto');
            option.innerHTML = 'auto';
            mainRootList.appendChild(option);
            let availableComponents = document.querySelectorAll('.component');
            for (let i = 0; i < availableComponents.length; i++) {
                let availableComponent = availableComponents[i];
                let option = document.createElement('option');
                option.setAttribute('value', availableComponent.getAttribute('data-component-id'));
                option.innerHTML = availableComponent.getAttribute('data-component-name');
                mainRootList.appendChild(option);
            }
            document.getElementById('main-root-label').classList.remove('hidden');
            mainRootList.classList.remove('hidden');
            if (options.mainRoot) {
                mainRootList.value = options.mainRoot;
            }
            document.getElementById('loading-screen').classList.add('hidden');
        });
    } else {
        document.getElementById('loading-text').innerHTML = 'File Not Supported';
    }
};

document.addEventListener('change', function(e) {
    if (e.target.getAttribute('id') === 'file-upload') {
        document.getElementById('redraw').removeAttribute('disabled');
        draw();
    }
});

document.addEventListener('click', function(event) {
    if (event.button !== 0) { 
        return;
    }
    if (event.target.id === 'redraw') {
        draw();
    } else if (event.target.classList.contains('component')) {
        for (let element of document.querySelectorAll('.component')) {
            element.classList.remove('highlighted-component');
        }
        event.target.classList.add('highlighted-component');
        document.getElementById('main-root').value = event.target.getAttribute('data-component-id');
        document.querySelector('#component-properties .name').innerHTML = event.target.getAttribute('data-component-name');
        document.querySelector('#component-properties .id').innerHTML = event.target.getAttribute('data-component-id');
        document.querySelector('#component-properties .type').innerHTML = event.target.getAttribute('data-component-type');

    } else if (event.target.id === 'save') {
        var scaleSVG = document.getElementById('dragSVG');
        var boundingRect = scaleSVG.getBBox();
        var svgContainer = document.createElement('svg');
        svgContainer.setAttribute('width', (boundingRect.width + boundingRect.x));
        svgContainer.setAttribute('height', (boundingRect.height + boundingRect.y));
        svgContainer.setAttribute('xmlns', 'http:
        svgContainer.innerHTML = document.getElementById('dragSVG').innerHTML;
        var a = document.createElement('a'),
        file = new Blob([svgContainer.outerHTML], {type: 'svg'});
        var url = URL.createObjectURL(file);
        a.href = url;
        a.download = 'pipeline.svg';
        document.body.appendChild(a);
        a.click();
        setTimeout(function() {
            document.body.removeChild(a);
            window.URL.revokeObjectURL(url);
        }, 0);
    }

});

document.addEventListener('click', function(event) {
    if (event.button !== 0) { 
        return;
    }
    let componentProperties = document.getElementById('component-properties');
    if (event.target.classList.contains('component')) {
        componentProperties.classList.remove('hidden');
        componentProperties.querySelector('.name').innerHTML = event.target.getAttribute('data-component-name');
        componentProperties.querySelector('.id').innerHTML = event.target.getAttribute('data-component-id');
        componentProperties.querySelector('.type').innerHTML = event.target.getAttribute('data-component-type');
    } else {
        componentProperties.classList.add('hidden');
    }
});
