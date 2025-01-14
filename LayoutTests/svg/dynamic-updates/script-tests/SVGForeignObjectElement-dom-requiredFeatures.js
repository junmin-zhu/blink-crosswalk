// [Name] SVGForeignObjectElement-dom-requiredFeatures.js
// [Expected rendering result] a series of PASS messages

createSVGTestCase();

var foreignObjectElement = createSVGElement("foreignObject");
foreignObjectElement.setAttribute("width", "200");
foreignObjectElement.setAttribute("height", "200");

var htmlDivElement = document.createElementNS(xhtmlNS, "xhtml:div");
htmlDivElement.setAttribute("style", "background-color: green; color: white; text-align: center");
htmlDivElement.textContent = "Test passed";

foreignObjectElement.appendChild(htmlDivElement);
rootSVGElement.appendChild(foreignObjectElement);

function repaintTest() {
    debug("Check that SVGForeignObjectElement is initially displayed");
    shouldHaveBBox("foreignObjectElement", "200", "200");
    debug("Check that setting requiredFeatures to something invalid makes it not render");
    foreignObjectElement.setAttribute("requiredFeatures", "http://www.w3.org/TR/SVG11/feature#BogusFeature");
    shouldHaveBBox("foreignObjectElement", "0", "0");
    debug("Check that setting requiredFeatures to something valid makes it render again");
    foreignObjectElement.setAttribute("requiredFeatures", "http://www.w3.org/TR/SVG11/feature#Shape");
    shouldHaveBBox("foreignObjectElement", "200", "200");
    debug("Check that adding something valid to requiredFeatures keeps rendering the element");
    foreignObjectElement.setAttribute("requiredFeatures", "http://www.w3.org/TR/SVG11/feature#Gradient");
    shouldHaveBBox("foreignObjectElement", "200", "200");
    debug("Check that adding something invalid to requiredFeatures makes it not render");
    foreignObjectElement.setAttribute("requiredFeatures", "http://www.w3.org/TR/SVG11/feature#BogusFeature");
    shouldHaveBBox("foreignObjectElement", "0", "0");

    completeTest();
}

var successfullyParsed = true;
