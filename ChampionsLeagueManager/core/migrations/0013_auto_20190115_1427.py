# Generated by Django 2.1.4 on 2019-01-15 13:27

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('core', '0012_substitution_minute'),
    ]

    operations = [
        migrations.AlterField(
            model_name='presence',
            name='is_in_starting_XI',
            field=models.BooleanField(default=True),
        ),
    ]
