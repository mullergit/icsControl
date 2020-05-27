
      var Timer_UdpateMesures;
      
      $('a[data-toggle=\"tab\"]').on('shown.bs.tab', function (e) {   
        //On supprime tous les timers lorsqu'on change d'onglet
        clearTimeout(Timer_UdpateMesures);  
        var target = $(e.target).attr("href")  
        console.log('activated ' + target );  
      
        // IE10, Firefox, Chrome, etc.
        if (history.pushState) 
          window.history.pushState(null, null, target);
        else 
          window.location.hash = target;        
        if (target=='#tab_mesures')  {
          $('#table_mesures').bootstrapTable('refresh',{silent:true, url:'/tabmesures.json'}); 
        }  
      });
      //$('body').on('click', '.change-style-menu-item', function()

      // Create a timer than update data every n seconds
      $('#tab_mesures').on('load-success.bs.table',function (e,data){
        console.log("tab_mesures loaded");
        if ($('.nav-tabs .active > a').attr('href')=='#tab_mesures') {
          Timer_UdpateMesures=setTimeout(function(){
            $('#table_mesures').bootstrapTable('refresh',{silent: true, showLoading: false, url: '/mesures.json'});//'/tabmesures.json'});
            updateMesures();
          },30000);
        }                 
      });   
          
      function updateMesures(){
        $.getJSON('/mesures.json', function(data){
          console.log("Mesures envoyees : " + JSON.stringify(data) + "|" + data.t + "|" + data.h + "|" + data.pa) ;
          $('#temperature').html(data.t);
          $('#humidite').html(data.h);
          $('#pa').html(data.pa); 
        }).fail(function(err){
          console.log("err getJSON mesures.json "+JSON.stringify(err));
        });
      };
      
      function labelFormatter(value, row){
        //console.log("labelFormatter");
        //console.log(value);
        //console.log(row);
        var label = "";
        if ( value === "Température" ) {
          label = value + "<span class='glyphicon " + row.glyph + " pull-left'></span>";
        } else if ( value === "Humidité" ) {
          label = value + "<span class='glyphicon " + row.glyph + " pull-left'></span>";
        } else if ( value === "Pression Atmosphérique" ) {
          label = value + "<span class='glyphicon " + row.glyph + " pull-left'></span>";
        } else {
          label = value;
        } 
        return label;
      }
      function valueFormatter(value, row){
        console.log("valueFormatter");
        var label = "";
        if ( row.valeur > row.precedente ) {
          label = value + row.unite + "<span class='glyphicon glyphicon-chevron-up pull-right'></span>";
        } else { 
          label = value + row.unite + "<span class='glyphicon glyphicon-chevron-down pull-right'></span>";
        }
        return label;
      }
      function vpFormatter(value, row){
        console.log("vpFormatter");
        var label = "";
        if ( row.valeur > row.precedente ) {
          label = value + row.unite
        } else { 
          label = value + row.unite
        }
        return label;
      }  

      $('#bt_iniciarConfig').click(function(){ initACconfig(); });

      function ChangeColorBtn2() {  
        document.getElementById('btn2').style.backgroundColor='Red';          
   }
      function initACconfig(){
        $.getJSON('/initACconfig.json', function(data){
          console.log("GET(initACconfig )" );
         /* var id = "#statusConfig";
          console.log(id);
          $(id).html('Aguardando envio dos códigos');*/

          console.log("statusConfig : " + JSON.stringify(data)) ;
          $('#statusConfig').html(data.status);
        }).fail(function(err){
          console.log("err GET(initACconfig");
        });
      }; 
      
      // Commandes sur le GPIO - GPIO change
      $('#D8_On').click(function(){ setBouton('D8','1'); });
      $('#D8_Off').click(function(){ setBouton('D8','0'); });
      $('#D6_On').click(function(){ setBouton('D6','1'); });
      $('#D6_Off').click(function(){ setBouton('D6','0'); });
      
      function setBouton(id, etat){
        $.post("gpio?id=" + id + "&etat=" + etat).done(function(data){
          console.log("Retour setBouton " + JSON.stringify(data)); 
          var id_gpio = "#" + id + "_etat";
          console.log(id_gpio);
          if ( data.success === "1" ) {
            if ( data.etat === "1" ) {
              $(id_gpio).html("ON");
            } else {
              $(id_gpio).html("OFF");
            }  
          } else {
            $(id_gpio).html('!');
          }      
        }).fail(function(err){
          console.log("err setButton " + JSON.stringify(err));
        });
      } 
      
      // Changement du thème - Change current theme
      // Adapté de - Adapted from : https://wdtz.org/bootswatch-theme-selector.html
      var supports_storage = supports_html5_storage();
      if (supports_storage) {
        var theme = localStorage.theme;
        console.log("Recharge le theme " + theme);
        if (theme) {
          set_theme(get_themeUrl(theme));
        }
      }
      
      // Un nouveau thème est sélectionné - New theme selected
      jQuery(function($){
        $('body').on('click', '.change-style-menu-item', function() {
          var theme_name = $(this).attr('rel');
          console.log("Changement de theme " + theme_name);
          var theme_url = get_themeUrl(theme_name);
          console.log("URL theme : " + theme_url);
          set_theme(theme_url);
        });
      });
      // Recupere l'adresse du theme - Get theme URL
      function get_themeUrl(theme_name){
        $('#labelTheme').html("Th&egrave;me : " + theme_name);
        var url_theme = "";
        if ( theme_name === "bootstrap" ) {
          url_theme = "https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css";
        } else {
          url_theme = "https://maxcdn.bootstrapcdn.com/bootswatch/3.3.7/" + theme_name + "/bootstrap.min.css";
        }
        if (supports_storage) {
          // Enregistre le theme sélectionné en local - save into the local database the selected theme
          localStorage.theme = theme_name;
        }
        return url_theme;
      }
      // Applique le thème - Apply theme
      function set_theme(theme_url) {
        $('link[title="main"]').attr('href', theme_url);
      }
      // Stockage local disponible ? - local storage available ?
      function supports_html5_storage(){
        try {
          return 'localStorage' in window && window['localStorage'] !== null;
        } catch (e) {
          return false;
        }
      }
