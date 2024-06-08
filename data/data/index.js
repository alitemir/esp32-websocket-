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

  $("#restart").on("click", () => {
    fetch("/restart");
  });

  $("#gecikme").on("click", () => {
    var sure = $("#geriSure").val();
    var ilerisure = $("#ilerisure").val();
    var gecikme = $("#gecikmeVal").val();

    var xhr = new XMLHttpRequest();
    xhr.open(
      "POST",
      `/manualmode?sure=${sure}&ilerisure=${ilerisure}&gecikme=${gecikme}`,
      true
    );
    xhr.send();
  });

  $("#savemodes").on("click", () => {
    var first = $("#1modeaktifinput").val();
    var second = $("#2modeaktifinput").val();
    var wintermode = $("#modewinter")[0].checked ? 1 : 0;
    var xhr = new XMLHttpRequest();
    xhr.open(
      "POST",
      `/wintermode?winter=${wintermode}&mode1=${first}&mode2=${second}`,
      true
    );
    xhr.send();
  });

  fetch("/winterstatus")
    .then((response) => response.json())
    .then((res) => {
      let kis = res["kismodu"] == "1";
      let bir = res["birinci"];
      let iki = res["ikinci"];
      let ileri = res["ileri"];
      let geri = res["geri"];
      let gecikme = res["gecikme"];
      let clk_active = res["clk_active"] == "1";
      let hr = res["hr"];
      let min = res["min"];
      let cr_fwd = res["cr_fwd"];
      let cr_back = res["cr_back"];
      let encoder = res["encoder"] == "1";
      let gerilimlimit = res["gerilimlimit"];
      let sarjlimlimit = res["sarjlimlimit"];
      let gerilimkalb = res["gerilimkalb"];
      let sarjkalb = res["sarjkalb"];
      let ilerikalb = res["ileriKalb"];
      let gerikalb = res["geriKalb"];
      let sinyaltime = res["sinyaltime"];
      let sinyalfark = res["sinyalfark"];




      $(`#modeaktif`).val(kis);
      $(`#1modeaktifinput`).val(bir);
      $(`#2modeaktifinput`).val(iki);
      $(`#ilerisure`).val(ileri);
      $(`#geriSure`).val(geri);
      $(`#gecikmeVal`).val(gecikme);
      $(`#modewinter`).prop("checked", kis);
      $(`#cbsysclockactive`).prop("checked", clk_active);
      $(`#hour`).val(hr);
      $(`#minute`).val(min);
      $(`#current_fwd`).val(cr_fwd);
      $(`#current_back`).val(cr_back);
      $(`#encoder`).prop("checked", encoder);
      $(`#voltagelimit`).val(gerilimlimit);
      $(`#chargelimit`).val(sarjlimlimit);
      $(`#voltagekalb`).val(gerilimkalb);
      $(`#chargekalb`).val(sarjkalb);
      $(`#ileridly`).val(ilerikalb);
      $(`#geridly`).val(gerikalb);
      $(`#loctime`).val(sinyaltime);
      $(`#locfark`).val(sinyalfark);


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

  $("#sysclock").on("click", () => {
    var is_enabled = $("#cbsysclockactive")[0].checked ? 1 : 0;
    var hour = $("#hour").val();
    var minute = $("#minute").val();
    var xhr = new XMLHttpRequest();
    xhr.open(
      "POST",
      `/sysclock?active=${is_enabled}&hour=${hour}&minute=${minute}`,
      true
    );
    xhr.send();
  });

  $("#savecurrets").on("click", () => {

    var curerntfwd = $("#current_fwd").val();
    var currentback = $("#current_back").val();
    var xhr = new XMLHttpRequest();
    xhr.open(
      "POST",
      `/currentctrl?current_fwd=${curerntfwd}&current_back=${currentback}`,
      true
    );
    xhr.send();
  });


  $("#savecharge").on("click", () => {
    var voltagelimit = $("#voltagelimit").val();
    var chargelimit = $("#chargelimit").val();
    var xhr = new XMLHttpRequest();
    xhr.open(
      "POST",
      `/chargectrl?voltagelimit=${voltagelimit}&chargelimit=${chargelimit}`,
      true
    );
    xhr.send();
  });
  $("#savekalb").on("click", () => {
    var voltagekalb = $("#voltagekalb").val();
    var chargekalb = $("#chargekalb").val();
    var xhr = new XMLHttpRequest();
    xhr.open(
      "POST",
      `/chargeali?voltagekalb=${voltagekalb}&chargekalb=${chargekalb}`,
      true
    );
    xhr.send();
  });

  $("#savesurekalb").on("click", () => {
    var ileridly = $("#ileridly").val();
    var geridly = $("#geridly").val();
    var xhr = new XMLHttpRequest();
    xhr.open(
      "POST",
      `/surekalb?ilerikalb=${ileridly}&gerikalb=${geridly}`,
      true
    );
    xhr.send();
  });

  $("#savelockalb").on("click", () => {
    var loctime = $("#loctime").val();
    var locfark = $("#locfark").val();
    var xhr = new XMLHttpRequest();
    xhr.open(
      "POST",
      `/lockalb?loctime=${loctime}&locfark=${locfark}`,
      true
    );
    xhr.send();
  });


  $("#saveencoder").on("click", () => {
    var enc = $("#encoder")[0].checked ? 1 : 0;
    var xhr = new XMLHttpRequest();
    xhr.open(
      "POST",
      `/encoder?enc=${enc}`,
      true
    );
    xhr.send();
  });


  setInterval(() => {
    fetch("/status")
      .then((response) => response.json())
      .then((stats) => {
        let { volt, amp, sigstr, status } = stats;
        $(`#statustext`).text(status);
        $("#0").addClass("d-none");
        $("#1").addClass("d-none");
        $("#2").addClass("d-none");
        $("#3").addClass("d-none");
        $("#4").addClass("d-none");
        $("#5").addClass("d-none");
        $("#6").addClass("d-none");
        $("#7").addClass("d-none");
        $("#8").addClass("d-none");
        $("#9").addClass("d-none");
        $("#10").addClass("d-none");
        $("#11").addClass("d-none");
        $("#12").addClass("d-none");
        $("#13").addClass("d-none");

        let idValue = Number(stats["status_code"]);
        if (idValue == 0) {
          $("#0").removeClass("d-none");
        } else if (idValue == 1) {
          $("#1").removeClass("d-none");
        } else if (idValue == 2) {
          $("#2").removeClass("d-none");
        } else if (idValue == 3) {
          $("#3").removeClass("d-none");
        } else if (idValue == 4) {
          $("#4").removeClass("d-none");
        } else if (idValue == 5) {
          $("#5").removeClass("d-none");
        } else if (idValue == 6) {
          $("#6").removeClass("d-none");
        } else if (idValue == 7) {
          $("#7").removeClass("d-none");
        } else if (idValue == 8) {
          $("#8").removeClass("d-none");
        } else if (idValue == 9) {
          $("#9").removeClass("d-none");
        } else if (idValue == 10) {
          $("#10").removeClass("d-none");
        } else if (idValue == 11) {
          $("#11").removeClass("d-none");
        } else if (idValue == 12) {
          $("#12").removeClass("d-none");
        } else if (idValue == 13) {
          $("#13").removeClass("d-none");
        }

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

        let signalstr = $("#signalstr");
        signalstr.text(sigstr);
        if (Number(sigstr) < 1000 && Number(sigstr) > 0) {
          signalstr.addClass("text-success");
          signalstr.removeClass("text-warning");
          signalstr.removeClass("text-danger");
        } else if (Number(sigstr) > 1000 || Number(sigstr) <= 3000) {
          signalstr.addClass("text-danger");
          signalstr.removeClass("text-success");
          signalstr.removeClass("text-warning");

        } else if (Number(sigstr) == -1000 || Number(sigstr) > 3000) {

          signalstr.addClass("text-warning");
          signalstr.removeClass("text-success");
          signalstr.removeClass("text-danger");

        }
        $("#saat").text(stats["hr"]);
        $("#dakika").text(stats["min"]);
        $("#saniye").text(stats["sec"]);
      });
  }, 1000);
});
