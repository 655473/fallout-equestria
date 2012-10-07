<rml>
  <head>
    <title>World Map</title>
    <link type="text/template" href="tpl_mainbar.rml" />
    <style>

#main
{
  background-color: black;
}

#map-frame
{
  position:   absolute;
  top:        0px;
  left:       0px;
  width:      80%;
  height:     80%;
  overflow: scroll;
}

#city-list
{
  position: absolute;
  top:    0px;
  right:  0px;
  width:  20%;
  height: 80%;
}

#main-bar
{
  position: absolute;
  bottom: 0px;
  left:   0px;
  width:  100%;
  height: 20%;
}

#party-cursor
{
  position: absolute;
  top:      0px;
  left:     0px;
  height:   10px;
  width:    10px;
  background-color: red;
  z-index:  998;
}

#{RCSS}


    </style>
  </head>


  <body id="body">

  <div id="main" style="width:100%; height:100%;">

    <div id="map-frame">
      <div id="party-cursor">-</div>
      <p id="pworldmap">
        #{RML}
      </p>
    </div>

    <div id="city-list">
      City list
    </div>

    <div id="main-bar">
      <button id="button-inventory">Inventory</button><br />
      <button id="button-character">Character</button><br />
      <button id="button-menu">Menu</button>
    </div>

  </div>

  </body>
</rml>