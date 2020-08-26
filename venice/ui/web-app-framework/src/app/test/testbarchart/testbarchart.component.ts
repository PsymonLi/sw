import { Component, OnInit } from '@angular/core';

@Component({
  selector: 'app-testbarchart',
  templateUrl: './testbarchart.component.html',
  styleUrls: ['./testbarchart.component.css']
})
export class TestbarchartComponent implements OnInit {

  public chartData: Array<any>;

  constructor() { }

  ngOnInit() {
    // give everything a chance to get loaded before starting the animation to reduce choppiness
    setTimeout(() => {
      this.generateData();

      // change the data periodically
      setInterval(() => this.generateData(), 3000);
    }, 1000);
  }

  generateData() {
    this.chartData = [];
    for (let i = 0; i < (8 + Math.floor(Math.random() * 10)); i++) {
      this.chartData.push([
        `Index ${i}`,
        Math.floor(Math.random() * 100)
      ]);
    }
  }

}
