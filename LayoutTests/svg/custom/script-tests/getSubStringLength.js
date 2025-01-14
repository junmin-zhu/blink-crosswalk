var svgNS = "http://www.w3.org/2000/svg";

var svgRoot = document.createElementNS(svgNS, "svg");
document.documentElement.appendChild(svgRoot);

var svgText = document.createElementNS(svgNS, "text");
svgText.style.fontFamily = "Ahem";
svgText.style.fontSize = "20px";
svgText.appendChild(document.createTextNode("abc"));
svgRoot.appendChild(svgText);

shouldThrow("svgText.getSubStringLength(-1, 2)");
shouldThrow("svgText.getSubStringLength(-1, 0)");
shouldBe("svgText.getSubStringLength(1, 3)", "40");
shouldBe("svgText.getSubStringLength(0, 4)", "60");
shouldThrow("svgText.getSubStringLength(3, 0)");

shouldBe("svgText.getSubStringLength(0, 0)", "0");
shouldBe("svgText.getSubStringLength(2, 0)", "0");

shouldBe("svgText.getSubStringLength(0, 1)", "20");
shouldBe("svgText.getSubStringLength(1, 1)", "20");
shouldBe("svgText.getSubStringLength(2, 1)", "20");
shouldBe("svgText.getSubStringLength(0, 3)", "60");

shouldNotThrow("svgText.getSubStringLength(1, -1)");
shouldNotThrow("svgText.getSubStringLength(2, -1)");
shouldThrow("svgText.getSubStringLength(3, -1)", "'IndexSizeError: Index or size was negative, or greater than the allowed value.'");
shouldThrow("svgText.getSubStringLength(3, -3)", "'IndexSizeError: Index or size was negative, or greater than the allowed value.'");

// cleanup
document.documentElement.removeChild(svgRoot);

var successfullyParsed = true;
