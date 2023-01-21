static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>ITECH Gübre Sıyırma Robotu</title>
    <link
      rel="stylesheet"
      href="https://cdn.jsdelivr.net/npm/bootstrap@4.1.3/dist/css/bootstrap.min.css"
      integrity="sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO"
      crossorigin="anonymous"
    />
    <script
      src="https://code.jquery.com/jquery-3.3.1.slim.min.js"
      integrity="sha384-q8i/X+965DzO0rT7abK41JStQIAqVgRVzpbzo5smXKp4YfRvH+8abtTE1Pi6jizo"
      crossorigin="anonymous"
    ></script>
    <script
      src="https://cdn.jsdelivr.net/npm/bootstrap@4.1.3/dist/js/bootstrap.min.js"
      integrity="sha384-ChfqqxuZUCnJSK3+MXmPNIyE6ZbWh2IMqE241rYiqJxyMiZ6OW/JmZQ5stwEULTy"
      crossorigin="anonymous"
    ></script>
    <meta
      charset="UTF-8"
      name="viewport"
      content="width=device-width, initial-scale=1"
    />
  </head>

  <body class="">
    <div class="container text-center">
      <h2 class="my-3">ITECH Gübre Sıyırma Robotu</h2>

      <div class="row mb-4">
        <div class="d-flex justify-content-center col-md-6">
          <div class="directional-buttons">
            <!-- <button
              class="direction-button up"
              onmousedown="toggleCheckbox('forward');"
              ontouchstart="toggleCheckbox('forward');"
              onmouseup="toggleCheckbox('x');"
              ontouchend="toggleCheckbox('x');"
            >
              <span class="visually-hidden">up</span>
            </button> -->
            <!-- <button
              class="direction-button left"
              onmousedown="toggleCheckbox('left');"
              ontouchstart="toggleCheckbox('left');"
              onmouseup="toggleCheckbox('x');"
              ontouchend="toggleCheckbox('x');"
            >
              <span class="visually-hidden">left</span>
            </button>
            <button
              class="direction-button right"
              onmousedown="toggleCheckbox('right');"
              ontouchstart="toggleCheckbox('right');"
              onmouseup="toggleCheckbox('x');"
              ontouchend="toggleCheckbox('x');"
            >
              <span class="visually-hidden">right</span>
            </button> -->
            <!-- <button
              class="direction-button down"
              onmousedown="toggleCheckbox('backward');"
              ontouchstart="toggleCheckbox('backward');"
              onmouseup="toggleCheckbox('x');"
              ontouchend="toggleCheckbox('x');"
            >
              <span class="visually-hidden">down</span>
            </button> -->

            <button
            class="direction-button left"
            onmousedown="toggleCheckbox('forward');"
            ontouchstart="toggleCheckbox('forward');"
            onmouseup="toggleCheckbox('x');"
            ontouchend="toggleCheckbox('x');"
          >
            <span class="visually-hidden">left</span>
          </button>

          <button
            class="direction-button right"
            onmousedown="toggleCheckbox('backward');"
            ontouchstart="toggleCheckbox('backward');"
            onmouseup="toggleCheckbox('x');"
            ontouchend="toggleCheckbox('x');"
          >
            <span class="visually-hidden">right</span>
          </button>
          </div>
        </div>
        <div class="d-flex justify-content-center col-md-6">
          <div class="directional-buttons">
            <button
              class="direction-button stop"
              onmousedown="toggleCheckbox('stop');"
              onmouseup="toggleCheckbox('x');"
            >
              <span class="text-white"> DUR </span>
              <span class="visually-hidden">stop</span>
            </button>
          </div>
        </div>
      </div>
      <div class="card p-2">
        <div class="row">
          <div class="col">
            <h2>Sistem Gerilimi</h2>
            <h1 id="voltage" class="dispay-1">~</h1>
          </div>
          <div class="col">
            <h2>Sistem Akimi</h2>
            <h1 id="amper" class="dispay-1">~</h1>
          </div>
        </div>
      </div>
      <div class="card">
        <h3 id="status" class="text-primary m-2">Durum: <i>Bekliyor</i></h3>
      </div>

      <div id="accordion">
        <div class="card">
          <div class="card-header">
            <h5>
              <button
                class="btn btn-link"
                data-toggle="collapse"
                data-target="#collapseOne"
              >
                Zamanlari Duzenle
              </button>
            </h5>
          </div>
          <div id="collapseOne" class="collapse" data-parent="#accordion">
            <div class="card-body">
              <div class="row">
                <div
                  class="custom-control custom-checkbox mx-3"
                  style="display: flex"
                >
                  <input
                    type="checkbox"
                    class="custom-control-input"
                    id="checkbox-1"
                  />
                  <label class="custom-control-label" for="checkbox-1"></label>
                </div>
                <input
                  id="clock-1"
                  class="col-md-2 col-sm-6 mr-4"
                  style="font-size: x-large"
                  type="time"
                  name="appt-time"
                  value="00:00"
                  step="3600"
                />
                <div
                  class="custom-control custom-checkbox mx-3"
                  style="display: flex"
                >
                  <input
                    type="checkbox"
                    class="custom-control-input"
                    id="checkbox-2"
                  />
                  <label class="custom-control-label" for="checkbox-2"></label>
                </div>
                <input
                  id="clock-2"
                  class="col-md-2 col-sm-6 mr-4"
                  style="font-size: x-large"
                  type="time"
                  name="appt-time"
                  value="00:00"
                  step="3600"
                />

                <div
                  class="custom-control custom-checkbox mx-3"
                  style="display: flex"
                >
                  <input
                    type="checkbox"
                    class="custom-control-input"
                    id="checkbox-3"
                  />
                  <label class="custom-control-label" for="checkbox-3"></label>
                </div>
                <input
                  id="clock-3"
                  class="col-md-2 col-sm-6 mr-4"
                  style="font-size: x-large"
                  type="time"
                  name="appt-time"
                  value="00:00"
                  step="3600"
                />

                <div
                  class="custom-control custom-checkbox mx-3"
                  style="display: flex"
                >
                  <input
                    type="checkbox"
                    class="custom-control-input"
                    id="checkbox-4"
                  />
                  <label class="custom-control-label" for="checkbox-4"></label>
                </div>
                <input
                  id="clock-4"
                  class="col-md-2 col-sm-6 mr-4"
                  style="font-size: x-large"
                  type="time"
                  name="appt-time"
                  value="00:00"
                  step="3600"
                />
              </div>
              <div class="row mt-3">
                <div
                  class="custom-control custom-checkbox mx-3"
                  style="display: flex"
                >
                  <input
                    type="checkbox"
                    class="custom-control-input"
                    id="checkbox-5"
                  />
                  <label class="custom-control-label" for="checkbox-5"></label>
                </div>
                <input
                  id="clock-5"
                  class="col-md-2 col-sm-6 mr-4"
                  style="font-size: x-large"
                  type="time"
                  name="appt-time"
                  value="00:00"
                  step="3600"
                />
                <div
                  class="custom-control custom-checkbox mx-3"
                  style="display: flex"
                >
                  <input
                    type="checkbox"
                    class="custom-control-input"
                    id="checkbox-6"
                  />
                  <label class="custom-control-label" for="checkbox-6"></label>
                </div>
                <input
                  id="clock-6"
                  class="col-md-2 col-sm-6 mr-4"
                  style="font-size: x-large"
                  type="time"
                  name="appt-time"
                  value="00:00"
                  step="3600"
                />

                <div
                  class="custom-control custom-checkbox mx-3"
                  style="display: flex"
                >
                  <input
                    type="checkbox"
                    class="custom-control-input"
                    id="checkbox-7"
                  />
                  <label class="custom-control-label" for="checkbox-7"></label>
                </div>
                <input
                  id="clock-7"
                  class="col-md-2 col-sm-6 mr-4"
                  style="font-size: x-large"
                  type="time"
                  name="appt-time"
                  value="00:00"
                  step="3600"
                />

                <div
                  class="custom-control custom-checkbox mx-3"
                  style="display: flex"
                >
                  <input
                    type="checkbox"
                    class="custom-control-input"
                    id="checkbox-8"
                  />
                  <label class="custom-control-label" for="checkbox-8"></label>
                </div>
                <input
                  id="clock-8"
                  class="col-md-2 col-sm-6 mr-4"
                  style="font-size: x-large"
                  type="time"
                  name="appt-time"
                  value="00:00"
                  step="3600"
                />
              </div>

              <div class="m-2">
                <button class="btn btn-success" id="time-save">
                  <svg
                    xmlns="http://www.w3.org/2000/svg"
                    width="16"
                    height="16"
                    fill="currentColor"
                    class="bi bi-save"
                    viewBox="0 0 16 16"
                  >
                    <path
                      d="M2 1a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1H9.5a1 1 0 0 0-1 1v7.293l2.646-2.647a.5.5 0 0 1 .708.708l-3.5 3.5a.5.5 0 0 1-.708 0l-3.5-3.5a.5.5 0 1 1 .708-.708L7.5 9.293V2a2 2 0 0 1 2-2H14a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2h2.5a.5.5 0 0 1 0 1H2z"
                    />
                  </svg>
                  Kaydet
                </button>
              </div>
            </div>
          </div>
        </div>
      </div>

      <div id="accordion">
        <div class="card">
          <div class="card-header">
            <h5>
              <button
                class="btn btn-link"
                data-toggle="collapse"
                data-target="#collapseTwo"
              >
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
          </div>
        </div>
      </div>

      <script>
        window.addEventListener("load", (event) => {
          fetch("/gettimes")
            .then((response) => response.json())
            .then((res) => {
              for (let index = 0; index < 8; index++) {
                let isEnabled = res[index][6] == "1";
                let value = res[index].substring(0, 5);
                $(`#clock-${index + 1}`).val(value);
                $(`#checkbox-${index + 1}`).prop("checked", isEnabled);
              }
            });
          $("#turIleri").on("click", () => {
            fetch("/turIleri");
          });
          $("#turGeri").on("click", () => {
            fetch("/turGeri");
          });
          $("#time-save").on("click", () => {
            let data = [];
            for (let index = 0; index < 8; index++) {
              const element = $(`#clock-${index + 1}`);
              const checkBox = $(`#checkbox-${index + 1}`)[0];
              let clock = element.val().split(":");
              let hours = Number(clock[0]);
              let minutes = Number(clock[1]);
              data.push([hours, minutes, checkBox.checked ? 1 : 0]);
            }
            timeSet(JSON.stringify(data));
          });
          setInterval(() => {
            fetch("/status")
              .then((response) => response.json())
              .then((stats) => {
                let { volt, amp, status } = stats;
                let voltage = $("#voltage");
                voltage.text(volt + "V");
                if (Number(volt) > 22 && Number(volt) < 27) {
                  voltage.addClass("text-success");
                  voltage.removeClass("text-warning");
                  voltage.removeClass("text-danger");
                } else if (Number(volt) > 30 || Number(volt) < 20) {
                  voltage.addClass("text-danger");
                  voltage.removeClass("text-success");
                  voltage.removeClass("text-warning");
                } else {
                  voltage.addClass("text-warning");
                  voltage.removeClass("text-success");
                  voltage.removeClass("text-danger");
                }

                let amper = $("#amper");
                amper.text(amp + "A");
                if (Number(amp) < 10 && Number(amp) > 0) {
                  amper.addClass("text-success");
                  amper.removeClass("text-warning");
                  amper.removeClass("text-danger");
                } else if (Number(amp) > 10 || Number(amp) < 13) {
                  amper.addClass("text-warning");
                  amper.removeClass("text-success");
                  amper.removeClass("text-danger");
                } else {
                  amper.addClass("text-danger");
                  amper.removeClass("text-success");
                  amper.removeClass("text-warning");
                }
                $("#status i").text(status);
                var checkboxes = document.querySelectorAll(
                  ".custom-control-input"
                );
                var clocks = document.querySelectorAll(".col-md-2.col-sm-6");

                checkboxes.forEach(function (checkbox, index) {
                  if (!checkbox.checked) {
                    clocks[index].style.backgroundColor = "red";
                  } else {
                    clocks[index].style.backgroundColor = "green";
                  }

                  checkbox.addEventListener("change", function () {
                    if (checkbox.checked) {
                      clocks[index].style.backgroundColor = "green";
                    } else {
                      clocks[index].style.backgroundColor = "red";
                    }
                  });
                });
              });
          }, 1000);
        });
      </script>
      <script>
        function toggleCheckbox(x) {
          var xhr = new XMLHttpRequest();
          xhr.open("GET", "/action?go=" + x, true);
          xhr.send();
        }

        function timeSet(x) {
          var xhr = new XMLHttpRequest();
          xhr.open("GET", "/action?time=" + x, true);
          xhr.send();
        }
      </script>
      <style>
        .custom-control-input:checked ~ .custom-control-label::before {
          background-color: green;
        }
        .custom-control-input:checked ~ .custom-control-label::after {
          background-color: green;
        }
        .custom-control-input:not(:checked) ~ .custom-control-label::before {
          background-color: red;
        }
        .custom-control-input {
          width: 30px;
          height: 30px;
        }
        .custom-control-label::before {
          width: 30px;
          height: 30px;
        }
        .custom-control-label::after {
          width: 30px;
          height: 30px;
        }
        .directional-buttons {
          width: 16rem;
          height: 16rem;
          display: grid;
          grid-gap: 0.5rem;
          grid-template-columns: 45fr 60fr 45fr;
          grid-template-rows: 45fr 60fr 45fr;
          grid-template-areas:
            "....  up  ....."
            "left  stop  right"
            ".... down .....";
        }

        .visually-hidden {
          position: absolute !important;
          height: 1px;
          width: 1px;
          overflow: hidden;
          clip: rect(1px 1px 1px 1px);
          clip: rect(1px, 1px, 1px, 1px);
          white-space: nowrap;
        }

        .direction-button {
          color: rgb(55, 50, 50);
          background: currentcolor;
          border: none;
          position: relative;
        }

        .direction-button:before {
          display: block;
          content: "";
          position: absolute;
          width: 4.25rem;
          height: 4.25rem;
          transform: rotate(45deg);
          background: currentcolor;
        }

        .direction-button:after {
          display: block;
          content: "";
          position: absolute;
          border: 2rem solid transparent;
          color: rgba(255, 250, 250, 0.6);

        }

        .direction-button:hover {
          color: rgb(221, 36, 36);
        }

        .direction-button:active:after {
          color: rgb(15, 11, 236);
        }

        .up {
          grid-area: up;
          border-radius: 0.5rem 0.5rem 0 0;
        }

        .up:before {
          left: calc(50% - 2.125rem);
          bottom: -2.125rem;
        }

        .up:after {
          border-bottom-color: currentcolor;
          left: calc(50% - 2rem);
          top: -1rem;
        }

        .left {
          grid-area: left;
          border-radius: 0.5rem 0 0 0.5rem;
        }

        .left:before {
          right: -2.125rem;
          top: calc(50% - 2.125rem);
        }

        .left:after {
          border-right-color: currentcolor;
          top: calc(50% - 2rem);
          left: -1rem;
        }

        .right {
          grid-area: right;
          border-radius: 0 0.5rem 0.5rem 0;
        }

        .right:before {
          left: -2.125rem;
          top: calc(50% - 2.125rem);
        }

        .right:after {
          border-left-color: currentcolor;
          top: calc(50% - 2rem);
          right: -1rem;
        }

        .down {
          grid-area: down;
          border-radius: 0 0 0.5rem 0.5rem;
        }

        .down:before {
          left: calc(50% - 2.125rem);
          top: -2.125rem;
        }

        .down:after {
          border-top-color: currentcolor;
          left: calc(50% - 2rem);
          bottom: -1rem;
        }

        .stop {
          grid-area: stop;
          border-radius: 1rem 1rem 1rem 1rem;
        }

        .stop:before {
          top: -1rem;
          display: none;
        }

        .stop:after {
          transform: rotate(90deg);
        }

        .stop:active {
          color: rgb(100, 19, 19);
        }

        .stop:active:after {
          color: rgb(255, 43, 43);
        }
      </style>
    </div>
  </body>
</html>
)rawliteral";