description('Test that setting and getting grid-definition-columns and grid-definition-rows works as expected');

debug("Test getting |grid-definition-columns| and |grid-definition-rows| set through CSS");
testGridDefinitionsValues(document.getElementById("gridWithFixedElement"), "7px 11px", "17px 2px");
testGridDefinitionsValues(document.getElementById("gridWithPercentElement"), "53% 99%", "27% 52%", "424px 792px", "162px 312px");
testGridDefinitionsValues(document.getElementById("gridWithAutoElement"), "auto auto", "auto auto", "0px 17px", "0px 3px");
testGridDefinitionsValues(document.getElementById("gridWithEMElement"), "100px 120px", "150px 170px");
testGridDefinitionsValues(document.getElementById("gridWithThreeItems"), "15px auto 100px", "120px 18px auto", "15px 0px 100px", "120px 18px 0px");
testGridDefinitionsValues(document.getElementById("gridWithPercentAndViewportPercent"), "50% 120px", "35% 168px", "400px 120px", "210px 168px");
testGridDefinitionsValues(document.getElementById("gridWithFitContentAndFitAvailable"), "none", "none");
testGridDefinitionsValues(document.getElementById("gridWithMinMaxContent"), "min-content max-content", "max-content min-content", "0px 0px", "0px 0px");
testGridDefinitionsValues(document.getElementById("gridWithMinMaxContentWithChildrenElement"), "min-content max-content", "max-content min-content", "7px 17px", "11px 3px");
testGridDefinitionsValues(document.getElementById("gridWithMinMaxAndFixed"), "minmax(45px, 30%) 15px", "120px minmax(35%, 10px)", "240px 15px", "120px 210px");
testGridDefinitionsValues(document.getElementById("gridWithMinMaxAndMinMaxContent"), "minmax(min-content, 30%) 15px", "120px minmax(35%, max-content)", "240px 15px", "120px 210px");
testGridDefinitionsValues(document.getElementById("gridWithFractionFraction"), "1fr 2fr", "3fr 4fr", "320px 480px", "225px 375px");
testGridDefinitionsValues(document.getElementById("gridWithFractionMinMax"), "minmax(min-content, 45px) 2fr", "3fr minmax(14px, max-content)", "45px 755px", "586px 14px");

debug("");
debug("Test the initial value");
var element = document.createElement("div");
document.body.appendChild(element);
shouldBe("getComputedStyle(element, '').getPropertyValue('grid-definition-columns')", "'none'");
shouldBe("getComputedStyle(element, '').getPropertyValue('grid-definition-rows')", "'none'");

debug("");
debug("Test getting and setting grid-definition-rows and grid-definition-columns through JS");
testGridDefinitionsSetJSValues("18px 22px", "66px 70px");
testGridDefinitionsSetJSValues("55% 80%", "40% 63%", "440px 640px", "240px 378px");
testGridDefinitionsSetJSValues("auto auto", "auto auto", "0px 0px", "0px 0px");
testGridDefinitionsSetJSValues("auto 16em 22px", "56% 10em auto", "0px 160px 22px", "336px 100px 0px");
testGridDefinitionsSetJSValues("16em minmax(16px, 20px)", "minmax(10%, 15%) auto", "160px 20px", "90px 0px");
testGridDefinitionsSetJSValues("16em 2fr", "14fr auto", "160px 640px", "600px 0px");
testGridDefinitionsSetJSValues("50% 12vw", "5% 85vh", "400px 96px", "30px 510px");

debug("");
debug("Test getting wrong values set from CSS");
var gridWithNoneAndAuto = document.getElementById("gridWithNoneAndAuto");
shouldBe("getComputedStyle(gridWithNoneAndAuto, '').getPropertyValue('grid-definition-columns')", "'none'");
shouldBe("getComputedStyle(gridWithNoneAndAuto, '').getPropertyValue('grid-definition-rows')", "'none'");

var gridWithNoneAndFixed = document.getElementById("gridWithNoneAndFixed");
shouldBe("getComputedStyle(gridWithNoneAndFixed, '').getPropertyValue('grid-definition-columns')", "'none'");
shouldBe("getComputedStyle(gridWithNoneAndFixed, '').getPropertyValue('grid-definition-rows')", "'none'");

debug("");
debug("Test setting and getting wrong values from JS");
testGridDefinitionsSetBadJSValues("none auto", "none auto");
testGridDefinitionsSetBadJSValues("none 16em", "none 56%");
testGridDefinitionsSetBadJSValues("none none", "none none");
testGridDefinitionsSetBadJSValues("auto none", "auto none");
testGridDefinitionsSetBadJSValues("auto none 16em", "auto 18em none");
testGridDefinitionsSetBadJSValues("-webkit-fit-content -webkit-fit-content", "-webkit-fit-available -webkit-fit-available");
testGridDefinitionsSetBadJSValues("auto minmax(16px, auto)", "minmax(auto, 15%) 10vw");
// Negative values are not allowed.
testGridDefinitionsSetBadJSValues("-10px minmax(16px, 32px)", "minmax(10%, 15%) -10vw");
testGridDefinitionsSetBadJSValues("10px minmax(16px, -1vw)", "minmax(-1%, 15%) 10vw");

function testInherit()
{
    var parentElement = document.createElement("div");
    document.body.appendChild(parentElement);
    parentElement.style.display = "grid";
    parentElement.style.width = "800px";
    parentElement.style.height = "600px";
    parentElement.style.font = "10px Ahem"; // Used to resolve em font consistently.
    parentElement.style.gridDefinitionColumns = "50px 1fr 'last'";
    parentElement.style.gridDefinitionRows = "2em 'middle' 45px";
    testGridDefinitionsValues(parentElement, "50px 750px last", "20px middle 45px");

    element = document.createElement("div");
    parentElement.appendChild(element);
    element.style.display = "grid";
    element.style.gridDefinitionColumns = "inherit";
    element.style.gridDefinitionRows = "inherit";
    testGridDefinitionsValues(element, "50px 0px last", "20px middle 45px");

    document.body.removeChild(parentElement);
}
debug("");
debug("Test setting grid-definition-columns and grid-definition-rows to 'inherit' through JS");
testInherit();

function testInitial()
{
    element = document.createElement("div");
    document.body.appendChild(element);
    element.style.display = "grid";
    element.style.width = "800px";
    element.style.height = "600px";
    element.style.gridDefinitionColumns = "150% 'middle' 55px";
    element.style.gridDefinitionRows = "1fr 'line' 2fr 'line'";
    testGridDefinitionsValues(element, "1200px middle 55px", "200px line 400px line");

    element.style.gridDefinitionColumns = "initial";
    element.style.gridDefinitionRows = "initial";
    shouldBe("getComputedStyle(element, '').getPropertyValue('grid-definition-columns')", "'none'");
    shouldBe("getComputedStyle(element, '').getPropertyValue('grid-definition-rows')", "'none'");

    document.body.removeChild(element);
}
debug("");
debug("Test setting grid-definition-columns and grid-definition-rows to 'initial' through JS");
testInitial();
