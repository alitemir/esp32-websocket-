static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html lang="tr">

<head>
  <title>ITECH Gübre Sıyırma Robotu</title>
  <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1" />

  <link rel="stylesheet" href="/data/style.css">
  <script src="/data/index.js"></script>

  <link rel="stylesheet" href="/data/bootstrap.min.css" crossorigin="anonymous" />
  
  <link href="/data/flag-icons.min.css" rel="stylesheet" />

  <script src="/data/jquery-3.6.0.min.js"></script>
  <script src="/data/popper.min.js"></script>
  <script src="/data/bootstrap.min.js"></script>

  <link href="/data/bootstrap4-toggle.min.css" rel="stylesheet">
  <script src="/data/bootstrap4-toggle.min.js"></script>

  


</head>

<body>

  <nav class="navbar navbar-expand-lg navbar-light bg-light">
    <div class="container">
    <a class="navbar-brand">
      <div class="row">
        <span class="d-block mx-3 font-weight-bold align-items-center ">Manuel Mod</span>
        <input id="toggle-silent" type="checkbox" data-toggle="toggle" onchange="sendMode();">
        <div id="console-event"></div>
      </div>
    </a>

    <button class="navbar-toggler" type="button" data-toggle="collapse" data-target="#navbarNav"
      aria-controls="navbarNav" aria-expanded="false" aria-label="Toggle navigation">
      <span class="navbar-toggler-icon"></span>
    </button>

    <div class="collapse navbar-collapse justify-content-end" id="navbarNav">
      <ul class="navbar-nav">
        <li class="nav-item dropdown">
          <a class="nav-link dropdown-toggle" id="language" data-toggle="dropdown" aria-haspopup="true"
            aria-expanded="false">
            <span class="flag-icon flag-icon-tr"></span> Türkçe
          </a>
          <div class="dropdown-menu" aria-labelledby="language">
            <a class="dropdown-item" href="gb/index.html"><span class="flag-icon flag-icon-gb"></span> English</a>
            <a class="dropdown-item" href="ru/index.html"><span class="flag-icon flag-icon-ru"></span> Русский</a>
          </div>
        </li>
      </ul>
    </div>
  </div>
  </nav>

  <main>
    <section class="mt-4">
      <h2 align="center">ITECH Gübre Sıyırma Robotu</h2>
      <div class="container" >
        <div class="row justify-content-around" >

          <div class="directional-buttons ">
            <button class="direction-button left"
              onmousedown="if (!document.getElementById('toggle-silent').unchecked) toggleCheckbox('forward');"
              ontouchstart="if (!document.getElementById('toggle-silent').unchecked) toggleCheckbox('forward');"
              onmouseup="if (!document.getElementById('toggle-silent').unchecked) toggleCheckbox('x');"
              ontouchend="if (!document.getElementById('toggle-silent').unchecked) toggleCheckbox('x');">
              <span class="visually-hidden">left</span>
            </button>
            <button class="direction-button right"
              onmousedown="if (!document.getElementById('toggle-silent').unchecked) toggleCheckbox('backward');"
              ontouchstart="if (!document.getElementById('toggle-silent').unchecked) toggleCheckbox('backward');"
              onmouseup="if (!document.getElementById('toggle-silent').unchecked) toggleCheckbox('x');"
              ontouchend="if (!document.getElementById('toggle-silent').unchecked) toggleCheckbox('x');">
              <span class="visually-hidden">right</span>
            </button>
          </div>
          
          <div class="directional-buttons">
            <button class="direction-button stop"
              onmousedown="if (!document.getElementById('toggle-silent').unchecked) toggleCheckbox('stop');"
              ontouchstart="if (!document.getElementById('toggle-silent').unchecked) toggleCheckbox('stop');"
              onmouseup="if (!document.getElementById('toggle-silent').unchecked) toggleCheckbox('x');"
              ontouchend="if (!document.getElementById('toggle-silent').unchecked) toggleCheckbox('x');"
              >
              <span class="text-white"> DUR </span>
              <span class="visually-hidden">stop</span>
            </button>
          </div>
        </div>
      </div>
    </section>
    
    <section>
      <div class="container text-center">
        <div class="card p-2">
          <div class="row">
            <div class="col">
              <h2>Sistem Gerilimi</h2>
              <h1 id="voltage" class="dispay-1">~</h1>
            </div>
            <div class="col">
              <h2>Sistem Akımı</h2>
              <h1 id="amper" class="dispay-1">~</h1>
            </div>
          </div>
        </div>
      </div>
    </section>
    
    <section>
      <div class="container text-center">
        <div class="card">
          <h3 id="status" class="text-primary m-2">Durum: <i>Bekliyor</i></h3>
        </div>
    </div>
    </section>
    
    <section id="accordion">
      <div class="container text-center">
        <div class="card">
          <div class="card-header">
            <h5>
              <button class="btn btn-link" data-toggle="collapse" data-target="#collapseOne">
                Zamanları Düzenle
              </button>
            </h5>
          </div>
          <div id="collapseOne" class="collapse" data-parent="#accordion">
            <div class="card-body">
              <div class="row justify-content-center">


                <div  class="timecolor mt-2">
                  <div class="custom-control custom-checkbox mx-3">
                    <input type="checkbox" class="custom-control-input" id="checkbox-1" />
                    <label class="custom-control-label" for="checkbox-1"></label>
                  </div>
                  <input id="clock-1" style="font-size: x-large;" type="time" name="appt-time" value="00:00" step="3600" />
                </div>
                
                <div  class="timecolor mt-2">
                  <div class="custom-control custom-checkbox mx-3">
                    <input type="checkbox" class="custom-control-input" id="checkbox-2" />
                    <label class="custom-control-label" for="checkbox-2"></label>
                  </div>
                  <input id="clock-2" style="font-size: x-large;" type="time" name="appt-time" value="00:00" step="3600" />
                </div>
                
                <div  class="timecolor mt-2">
                  <div class="custom-control custom-checkbox mx-3">
                    <input type="checkbox" class="custom-control-input" id="checkbox-3" />
                    <label class="custom-control-label" for="checkbox-3"></label>
                  </div>
                  <input id="clock-3" style="font-size: x-large;" type="time" name="appt-time" value="00:00" step="3600" />
                </div>
                
                <div  class="timecolor mt-2">
                  <div class="custom-control custom-checkbox mx-3">
                    <input type="checkbox" class="custom-control-input" id="checkbox-4" />
                    <label class="custom-control-label" for="checkbox-4"></label>
                  </div>
                  <input id="clock-4" style="font-size: x-large;" type="time" name="appt-time" value="00:00" step="3600" />
                </div>

                <div  class="timecolor mt-2">
                  <div class="custom-control custom-checkbox mx-3">
                    <input type="checkbox" class="custom-control-input" id="checkbox-5" />
                    <label class="custom-control-label" for="checkbox-5"></label>
                  </div>
                  <input id="clock-5" style="font-size: x-large;" type="time" name="appt-time" value="00:00" step="3600" />
                </div>

                <div  class="timecolor mt-2">
                  <div class="custom-control custom-checkbox mx-3">
                    <input type="checkbox" class="custom-control-input" id="checkbox-6" />
                    <label class="custom-control-label" for="checkbox-6"></label>
                  </div>
                  <input id="clock-6" style="font-size: x-large;" type="time" name="appt-time" value="00:00" step="3600" />
                </div>

                <div  class="timecolor mt-2">
                  <div class="custom-control custom-checkbox mx-3">
                    <input type="checkbox" class="custom-control-input" id="checkbox-7" />
                    <label class="custom-control-label" for="checkbox-7"></label>
                  </div>
                  <input id="clock-7" style="font-size: x-large;" type="time" name="appt-time" value="00:00" step="3600" />
                </div>

                <div  class="timecolor mt-2">
                  <div class="custom-control custom-checkbox mx-3">
                    <input type="checkbox" class="custom-control-input" id="checkbox-8" />
                    <label class="custom-control-label" for="checkbox-8"></label>
                  </div>
                  <input id="clock-8" style="font-size: x-large;" type="time" name="appt-time" value="00:00" step="3600" />
                </div>


                </div>

              <div class="m-2">
                <button class="btn btn-success" id="time-save">
                  <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-save"
                    viewBox="0 0 16 16">
                    <path
                      d="M2 1a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1H9.5a1 1 0 0 0-1 1v7.293l2.646-2.647a.5.5 0 0 1 .708.708l-3.5 3.5a.5.5 0 0 1-.708 0l-3.5-3.5a.5.5 0 1 1 .708-.708L7.5 9.293V2a2 2 0 0 1 2-2H14a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2h2.5a.5.5 0 0 1 0 1H2z" />
                  </svg>
                  Kaydet
                </button>
              </div>
            </div>
          </div>
        </div>
      </div>
      <div class="container text-center">
        <div class="card">
          <div class="card-header">
            <h5>
              <button class="btn btn-link" data-toggle="collapse" data-target="#collapseTwo">
                Ayarlar
              </button>
            </h5>
          </div>
          <div id="collapseTwo" class="collapse" data-parent="#accordion">
            <div class="card-body">
              <div class="row d-flex justify-content-center p-2">
                <button id="turIleri" class="btn btn-warning m-2">
                  İleri Ek Tur
                </button>
                <button id="turGeri" class="btn btn-warning m-2">
                  Geri Ek Tur
                </button>
              </div>
            </div>
            <div class="form-group row">
              <div class="col-sm-6">
                <div class="custom-control custom-checkbox">
                  <input type="checkbox" class="custom-control-input" id="checkbox-temperature">
                  <label class="custom-control-label" for="checkbox-temperature">Kış Modu</label>
                </div>
              </div>
              <div class="col-sm-6">
                <div class="form-group">
                  <label for="temperature-range">Sıcaklık Aralığı</label>
                  <input type="range" class="form-control-range" id="temperature-range" min="0" max="100" step="1">
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>

  </main> 
  <footer>
    <div class="container text-center">
    <div class="copyright mt-3 ">
      Copyright © 2023 | I-TECH ROBOTICS
    </div>
  </div>
  </footer>


  <script src="/data/jquery-3.3.1.slim.min.js" crossorigin="anonymous"></script>

</body>

</html>
)rawliteral";