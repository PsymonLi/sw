import { CommonModule } from '@angular/common';
import { NgModule } from '@angular/core';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';
import { MaterialdesignModule } from '@lib/materialdesign.module';
import { LoginComponent } from './login.component';
import { loginRouter } from './login.router';



@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    ReactiveFormsModule,
    MaterialdesignModule,
    loginRouter
  ],
  declarations: [LoginComponent],
  exports: [LoginComponent, MaterialdesignModule]
})
export class LoginModule { }
